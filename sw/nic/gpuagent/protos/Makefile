export PROTOC ?= protoc
TOP_DIR = ../../..
GEN_DIR = ${TOP_DIR}/nic/gpuagent/gen/go
GPU_PROTO_FILES = $(shell ls ${CURDIR} | grep .proto | grep -v types.proto | grep -v module | tr " " "\n" | LC_ALL=C sort | tr "\n" " ")
$(info ${GPU_PROTO_FILES})

default: create-gen-dir gpu-proto

gpu-proto: gen-clean
	LC_ALL=C ${PROTOC} --proto_path=${CURDIR} -I${CURDIR} \
                                -I${TOP_DIR} -I$(TOP_DIR)/vendor/github.com/gogo/protobuf/gogoproto \
				                --gogofast_out=Mgoogle/protobuf/any.proto=github.com/gogo/protobuf/types,Mgogo.proto=github.com/gogo/protobuf/gogoproto,plugins=grpc:${GEN_DIR} types.proto ${GPU_PROTO_FILES}

create-gen-dir:
	mkdir -p ${GEN_DIR}

clean-proto:
	rm -f ${GEN_DIR}/*.go

gen-clean:
	rm -f ${GEN_DIR}/*.go

proto-sync: default
	@$(eval DIFFS=`git ls-files --exclude-standard --modified --directory ${CURDIR}`)
	@echo "Found the following uncommitted pb.go files, if any"
	@echo $(DIFFS)
	@test -z "$(DIFFS)"
