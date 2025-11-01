FROM ubuntu:24.04

RUN apt update && apt upgrade -y && apt install -y \
    build-essential \
    cmake \
    ninja-build \
    gdb \
    git \
    bash \
    vim \
    net-tools \
    netcat-openbsd \
    telnet \
    lsof \
    iproute2 \
    tcpdump \
    nmap \
    curl \
    wget \
    && apt clean && rm -rf /var/lib/apt/lists/*