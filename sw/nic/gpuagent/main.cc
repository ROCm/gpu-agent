
/*
Copyright (c) Advanced Micro Devices, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/


//----------------------------------------------------------------------------
///
/// \file
/// front-end agent for managing GPUs
///
//----------------------------------------------------------------------------

#define __STDC_FORMAT_MACROS
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <inttypes.h>
#include <thread>
#include <iostream>
#include <fstream>
#include <pthread.h>
#include <signal.h>
#include <cerrno>
#include <cstring>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <grpc++/grpc++.h>
#include <arpa/inet.h>
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/assert.hpp"
#include "nic/gpuagent/include/globals.hpp"
#include "nic/gpuagent/init.hpp"
#include "nic/gpuagent/api/include/aga_init.hpp"
#include "nic/gpuagent/svc/gpu.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

// global variable to store socket path for cleanup
static std::string g_socket_path;

/// \brief    print usage information
static void inline
print_usage (char **argv)
{
    fprintf(stdout, "Usage : %s [-p <port> | --grpc-server-port <port>] "
            "[-i <ip-addr> | --grpc-server-ip <ip-addr>] "
            "[-s <socket-path> | --grpc-unix-socket <socket-path>]\n\n",
            argv[0]);
    fprintf(stdout, "Options:\n");
    fprintf(stdout, "  -p, --grpc-server-port <port>       gRPC server port (default: %u)\n",
            AGA_DEFAULT_GRPC_SERVER_PORT);
    fprintf(stdout, "  -i, --grpc-server-ip <ip-addr>      gRPC server IP address (default: 127.0.0.1)\n");
    fprintf(stdout, "  -s, --grpc-unix-socket <path>       Use Unix socket instead of TCP/IP (default: %s)\n",
            AGA_DEFAULT_UNIX_SOCKET_PATH);
    fprintf(stdout, "Use -h | --help for help\n");
}

/// \brief    cleanup Unix socket file on exit
/// \param[in] socket_path
static void
clean_unix_socket (const std::string& socket_path)
{
    unlink(socket_path.c_str());
}

/// \brief    dedicated thread that waits for termination signals and
///           performs cleanup in normal thread context (async-signal-safe)
static void
signal_wait_thread (void)
{
    int sig;
    sigset_t sigset;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGTERM);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGABRT);
    sigaddset(&sigset, SIGQUIT);
    sigaddset(&sigset, SIGHUP);
    // block until a signal arrives
    sigwait(&sigset, &sig);
    // cleanup api layer
    aga_api_teardown();
    // cleanup unix socket if used
    if (!g_socket_path.empty()) {
        clean_unix_socket(g_socket_path);
    }
    // restore default handler, unblock the signal in this thread
    // (sigwait dequeues but does not unblock), then re-raise so the
    // process terminates with the correct signal/exit status
    signal(sig, SIG_DFL);
    sigemptyset(&sigset);
    sigaddset(&sigset, sig);
    pthread_sigmask(SIG_UNBLOCK, &sigset, NULL);
    raise(sig);
}

/// \brief    prepare Unix socket for gRPC server
/// \param[in] socket_path    Unix socket file path
/// \return   SDK_RET_OK on success, SDK_RET_ERR on failure
static sdk_ret_t
prepare_unix_socket (const std::string& socket_path)
{
    int test_sock;
    struct stat st;
    size_t last_slash;
    struct stat dir_st;
    socklen_t addr_len;
    std::string dir_path;
    struct sockaddr_un addr;

    // validate socket path length
    if (socket_path.length() >= sizeof(addr.sun_path)) {
        fprintf(stderr, "Error: Socket path too long (max %zu chars): %s\n",
                sizeof(addr.sun_path) - 1, socket_path.c_str());
        return SDK_RET_ERR;
    }

    // check if socket file exists
    if (stat(socket_path.c_str(), &st) == 0) {
        // socket file exists - check if it's in use
        if (S_ISSOCK(st.st_mode)) {
            // try to connect to see if another instance is running
            test_sock = socket(AF_UNIX, SOCK_STREAM, 0);
            if (test_sock >= 0) {
                memset(&addr, 0, sizeof(addr));
                addr.sun_family = AF_UNIX;
                strncpy(addr.sun_path, socket_path.c_str(),
                        sizeof(addr.sun_path) - 1);
                addr_len = offsetof(struct sockaddr_un, sun_path) +
                           strlen(addr.sun_path) + 1;

                if (connect(test_sock, (struct sockaddr*)&addr, addr_len) == 0) {
                    // another instance is running
                    close(test_sock);
                    fprintf(stderr, "Error: Another GPU agent instance is "
                            "already running on socket %s\n",
                            socket_path.c_str());
                    return SDK_RET_ERR;
                }
                close(test_sock);
            }

            // socket exists but not in use - clean it up
            if (unlink(socket_path.c_str()) != 0) {
                fprintf(stderr, "Error: Failed to remove stale socket file %s, "
                        "err - %s\n", socket_path.c_str(), strerror(errno));
                return SDK_RET_ERR;
            }
        } else {
            // file exists but is not a socket
            fprintf(stderr, "Error: Path %s exists but is not a socket\n",
                    socket_path.c_str());
            return SDK_RET_ERR;
        }
    }

    // ensure parent directory exists
    last_slash = socket_path.find_last_of('/');
    if (last_slash != std::string::npos) {
        dir_path = socket_path.substr(0, last_slash);
        if (stat(dir_path.c_str(), &dir_st) != 0) {
            // directory doesn't exist - create it
            if (mkdir(dir_path.c_str(), 0755) != 0 && errno != EEXIST) {
                fprintf(stderr, "Error: Failed to create directory %s, err - %s\n",
                        dir_path.c_str(), strerror(errno));
                return SDK_RET_ERR;
            }
        } else if (!S_ISDIR(dir_st.st_mode)) {
            fprintf(stderr, "Error: Path %s exists but is not a directory\n",
                    dir_path.c_str());
            return SDK_RET_ERR;
        }
    }
    return SDK_RET_OK;
}

int
main (int argc, char **argv)
{
    int oc, rv;
    sdk_ret_t ret;
    sigset_t sigset;
    struct in6_addr ip_addr;
    std::string grpc_server;
    std::string grpc_server_ip;
    std::string grpc_server_port;
    std::string grpc_unix_socket;
    bool use_unix_socket = false;
    aga_init_params_t init_params = {};
    // command line options
    struct option longopts[] = {
        { "grpc-server-port", required_argument, NULL, 'p' },
        { "grpc-server-ip",   required_argument, NULL, 'i' },
        { "grpc-unix-socket", optional_argument, NULL, 's' },
        { "help",             no_argument,       NULL, 'h' },
        { 0,                  0,                 NULL,  0  }
    };

    // parse CLI options
    while ((oc = getopt_long(argc, argv, ":hp:i:s:", longopts, NULL)) != -1) {
        switch (oc) {
        case 'p':
            try {
                int port = std::stoi(optarg);
                if ((port <= 0) || (port > 65535)) {
                    fprintf(stderr, "Invalid gRPC server port %d specified\n",
                            port);
                    print_usage(argv);
                    exit(1);
                }
            } catch (const std::invalid_argument &e) {
                fprintf(stderr, "Invalid gRPC server port specified\n");
                print_usage(argv);
                exit(1);
            }
            grpc_server_port = optarg;
            break;

        case 'i':
            if (inet_pton(AF_INET, optarg, &ip_addr) != 1) {
                if (inet_pton(AF_INET6, optarg, &ip_addr) != 1) {
                    fprintf(stderr, "Invalid gRPC server IP %s specified\n",
                            optarg);
                    print_usage(argv);
                    exit(1);
                }
            }
            grpc_server_ip = optarg;
            break;

        case 's':
            if (optarg) {
                grpc_unix_socket = optarg;
            } else {
                grpc_unix_socket = AGA_DEFAULT_UNIX_SOCKET_PATH;
            }
            use_unix_socket = true;
            break;

        case 'h':
            print_usage(argv);
            exit(0);
            break;

        default:
            // ignore all other options
            break;
        }
    }
    // determine gRPC server type and address
    if (use_unix_socket) {
        // use Unix socket
        grpc_server = "unix:" + grpc_unix_socket;
    } else {
        // use TCP/IP
        // use default IP for gRPC server if not specified
        if (grpc_server_ip.empty()) {
            grpc_server_ip = "127.0.0.1";
        }
        // use default port for gRPC server if not specified
        if (grpc_server_port.empty()) {
            grpc_server_port = std::to_string(AGA_DEFAULT_GRPC_SERVER_PORT);
        }
        grpc_server = grpc_server_ip + ":" + grpc_server_port;
    }
    // initialize the init params
    strncpy(init_params.grpc_server, grpc_server.c_str(),
            AGA_GRPC_SERVER_STR_LEN);
    init_params.grpc_server[AGA_GRPC_SERVER_STR_LEN - 1] = '\0';
    // block termination signals in all threads so the dedicated
    // signal-wait thread can handle them in normal context
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGTERM);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGABRT);
    sigaddset(&sigset, SIGQUIT);
    sigaddset(&sigset, SIGHUP);
    rv = pthread_sigmask(SIG_BLOCK, &sigset, NULL);
    if (rv) {
        fprintf(stderr, "Failed to block signals, err %d\n", rv);
    }
    // spawn signal-wait thread for async-signal-safe cleanup
    std::thread(signal_wait_thread).detach();
    // handle Unix socket setup before initialization
    if (use_unix_socket) {
        // prepare Unix socket (check for existing instances, cleanup, etc.)
        ret = prepare_unix_socket(grpc_unix_socket);
        if (ret != SDK_RET_OK) {
            exit(1);
        }
        // save socket path for cleanup
        g_socket_path = grpc_unix_socket;
    } else {
        // cleanup any existing Unix socket on TCP initialization
        clean_unix_socket(AGA_DEFAULT_UNIX_SOCKET_PATH);
    }
    // initialize the agent
    ret = aga_init(&init_params);
    SDK_ASSERT(ret == SDK_RET_OK);
    fprintf(stderr, "gRPC server exited, agent shutting down ...\n");
    // cleanup Unix socket on exit
    if (use_unix_socket && !g_socket_path.empty()) {
        clean_unix_socket(g_socket_path);
    }
    return 0;
}
