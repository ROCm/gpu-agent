
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
/// front-end agent for rdcd
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
#include <grpc++/grpc++.h>
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/assert.hpp"
#include "nic/gpuagent/include/globals.hpp"
#include "nic/gpuagent/init.hpp"
#include "nic/gpuagent/svc/gpu.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

/// \brief    print usage information
static void inline
print_usage (char **argv)
{
    fprintf(stdout, "Usage : %s [-p <port> | --grpc-server-port <port>]\n\n",
            argv[0]);
    fprintf(stdout, "Use -h | --help for help\n");
}

int
main (int argc, char **argv)
{
    int oc;
    sdk_ret_t ret;
    aga_init_params_t init_params = {};
    // command line options
    struct option longopts[] = {
        { "grpc-server-port", required_argument, NULL, 'p' },
        { "rdc-server",       required_argument, NULL, 's' },
        { "help",             no_argument,       NULL, 'h' },
        { 0,                  0,                 NULL,  0  }
    };

    // parse CLI options
    while ((oc = getopt_long(argc, argv, ":hp:s:W;", longopts, NULL)) != -1) {
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
            init_params.grpc_server = std::string("0.0.0.0:") + optarg;
            break;

        case 's':
            init_params.rdc_server = optarg;
            if (init_params.rdc_server.empty()) {
                fprintf(stderr, "rdcd server IP is not specified\n");
                print_usage(argv);
                exit(1);
            }
            init_params.rdc_server +=
                ":" + std::to_string(AGA_DEFAULT_RDC_GRPC_SERVER_PORT);
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
    // use default IP for rdc server if not specified
    if (init_params.rdc_server.empty()) {
        init_params.rdc_server =
            "127.0.0.1:" + std::to_string(AGA_DEFAULT_RDC_GRPC_SERVER_PORT);
    }
    // use default IP, port for gRPC server if not specified
    if (init_params.grpc_server.empty()) {
        init_params.grpc_server =
            "0.0.0.0:" + std::to_string(AGA_DEFAULT_GRPC_SERVER_PORT);
    }
    // initialize the agent
    ret = aga_init(&init_params);
    SDK_ASSERT(ret == SDK_RET_OK);
    fprintf(stderr, "gRPC server exited, agent shutting down ...\n");
    return 0;
}
