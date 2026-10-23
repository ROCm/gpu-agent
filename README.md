# GPU Agent provides programmable APIs to configure and monitor AMD Instinct GPUs

## To build GPU Agent, follow the steps below:

### setup workspace (required once)

```bash
$ git submodule update --init  --recursive -f
```

### create build container image (required once)

```bash
$ make build-container
```

### Building artifacts
  
  Follow either of the two methods below to build gpuagent and gpuctl binaries
  
  #### Manual Steps
  
  vendor setup workspace for manual building (required once)

  - choose build/developer environment
      - rhel9 
      ```bash
      [user@host]# GPUAGENT_BLD_CONTAINER_IMAGE=gpuagent-builder-rhel:9 make docker-shell
      ```

      - ubuntu 22.04
      ```bash
       [user@host]# GPUAGENT_BLD_CONTAINER_IMAGE=gpuagent-bldr-ubuntu:22.04 make docker-shell
      ```

  - golang dependency setup (required once)
      ```bash
      [user@build-container ]# make gopkglist
      ```
  - golang vendor setup
      ```bash
      [user@build-container ]# cd sw/nic/gpuagent/
      [user@build-container ]# go mod vendor
      ```

  - bild gpuagent (within build-container)
      ```bash
      [user@build-container ]# make
      ```

  #### Full target build in single step (from host)

  Choose build base os

  - rhel9
    ```bash
    [user@host]# GPUAGENT_BLD_CONTAINER_IMAGE=gpuagent-builder-rhel:9 make gpuagent
    ```

  - ubuntu 22.04
    ```bash
    [user@host]# GPUAGENT_BLD_CONTAINER_IMAGE=gpuagent-bldr-ubuntu:22.04 make gpuagent
    ```

### Artifacts location
  - gpuagent binary can be found at - ${TOP_DIR}/sw/nic/build/x86_64/sim/bin/gpuagent
  - gpuctl binary can be found at - ${TOP_DIR}/sw/nic/build/x86_64/sim/bin/gpuctl

### To clean the build artifacts (run it within build-container)

```bash
[root@dev gpu-agent]# make -C sw/nic/gpuagent clean
[root@dev gpu-agent]# 
```

## WSL2 build (gpuagent-wsl)

`gpuagent-wsl` is a variant of gpuagent that uses AMD's librocdxg amdsmi backend
to monitor GPUs from within Windows Subsystem for Linux 2 (WSL2).

### Prerequisites

- WSL2 kernel with `/dev/dxg` present (Windows 11 or Windows 10 22H2+).
- [rocdxg-amd-smi-lib](https://github.com/rocm-systems/librocdxg) installed at
  `/opt/rocm-wsl` (override with `WSL_AMD_SMI_LIB_DIR=<path>`).
- Third-party libs pre-built (run `make build-libs-docker` once if not already done).

### Build targets

| Target | Description |
|---|---|
| `make gpuagent_wsl` | Native WSL2 build (no Docker). Requires librocdxg at `/opt/rocm-wsl`. |
| `make gpuagent_wsl_docker` | Build inside the RHEL9 container with a stub `libamd_smi.so` — verifies compile and link without real hardware. |
| `make build-libs-docker` | Compile third-party libs inside the container (required once before `make gpuagent_wsl`). |
| `make wsl-shim` | Build the LD_PRELOAD shim (`wsl-shim/`). Install with `make -C wsl-shim install`. |

### WSL-shim

The wsl-shim (`wsl-shim/wsl_amdsmi_shim.cc`) is an LD_PRELOAD shared library that
intercepts `libamd_smi` calls and redirects them to librocdxg on WSL2. It is
installed to `/usr/local/metrics/lib/libamd_smi_shim.so`. Use it when running an
existing gpuagent binary on WSL2 without rebuilding:

```bash
LD_PRELOAD=/usr/local/metrics/lib/libamd_smi_shim.so gpuagent
```

### Known-unsupported metrics on WSL2

The following metrics are not available through librocdxg and will report as
unavailable (AMD SMI NA sentinel):

- `voltage_soc` (no VDDSOC sensor; only VDDGFX is exposed)
- `pcie_bandwidth_acc` (instantaneous Mb/s only; no accumulated byte counter)
- Violation statistics (throttling events)
- Fan speed

### Artifact location

```
sw/nic/build/x86_64/sim/bin/gpuagent-wsl
```

# Things to note
 - For updating any amdsmi library to any other version, make sure the libamdsmi.so libraries are built correctly and are available in sw/nic/build/x86_64/sim/lib/ path. These are required during runtime, mismatch in library version may lead to runtime issues. These libraries are built from [amdsmi git](https://github.com/rocm/amdsmi/). The commit/tag the current gpuagent is built on can be found in [file](sw/nic/third-party/rocm/amd_smi_lib/version.txt)
 - apply patches on amdsmi found in [here](patch/amdsmi)
 - amdsmi build instructions are available [here](sw/nic/gpuagent/api/smi/amdsmi/README.md)

# Troubleshooting
 - If you face any issue with golang dependencies, re-run `make gopkglist` and `go mod vendor` command.
 - some go files are generated during build time, if you face any issue related to missing files, run `make gpuagent` command within build-container, then re-run `go mod vendor` command.
