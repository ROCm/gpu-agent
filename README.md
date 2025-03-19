GPU Agent provides programmable APIs to configure and monitor AMD Instinct GPUs

# Build Instructions

## Create build container image (once)
```bash
make docker-build
```

## Compilation
```bash
make docker-compile
```

## Artifact location
```bash
ls sw/nic/build/x86_64/sim/bin/gpuagent
```

## Clean build artifacts
```bash
make clean
```
