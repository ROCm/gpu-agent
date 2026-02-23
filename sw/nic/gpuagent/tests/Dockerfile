FROM ubuntu:22.04

ENV PROTOBUF_VERSION=23.4
ENV GOPATH=/go
ENV GOROOT=/usr/local/go
ENV PATH=$PATH:$GOROOT/bin:$GOPATH/bin

RUN DEBIAN_FRONTEND=noninteractive \
    apt-get update && \
    apt-get install -y wget git make unzip build-essential && \
    echo 'Install golang' && \
    wget https://dl.google.com/go/go1.23.2.linux-amd64.tar.gz && \
    tar -C /usr/local -xzf go1.23.2.linux-amd64.tar.gz && \
    export PATH=/usr/local/go/bin:$PATH && \
    export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH && \
    echo 'export PATH=/usr/local/go/bin:$PATH' >> ~/.bashrc && \
    echo 'export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH' >> ~/.bashrc && \
    echo 'Install go protoc' && \
    export GOPATH=/usr/local && \
    export PATH=$PATH:/usr/local/go/bin:/root/go/bin && \
    wget "https://github.com/protocolbuffers/protobuf/releases/download/v${PROTOBUF_VERSION}/protoc-${PROTOBUF_VERSION}-linux-x86_64.zip" \
    && unzip "protoc-${PROTOBUF_VERSION}-linux-x86_64.zip" -d /usr/local \
    && rm "protoc-${PROTOBUF_VERSION}-linux-x86_64.zip" \
    rm -rf /var/lib/apt/lists/* && \
    rm -rf /usr/lib/locale && \
    rm -rf /usr/share/locale && \
    rm -rf /usr/local/src && \
    rm -rf /root/.cache && \
    apt-get clean && \
    apt-get autoclean

WORKDIR /gpu-agent
RUN go mod init temp

RUN go get github.com/gogo/protobuf/protoc-gen-gogofast
RUN go install github.com/gogo/protobuf/protoc-gen-gogofast

CMD ["/usr/bin/bash"]