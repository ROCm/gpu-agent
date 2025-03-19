#!/usr/bin/env bash
set -x
#set -eou pipefail
dir=/usr/src/github.com/ROCm/gpu-agent

cd $dir/sw/nic

make -C gpuagent
ls build/x86_64/sim/bin/gpuagent

echo "gpuagent build successfull"
