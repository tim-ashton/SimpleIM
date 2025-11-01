FROM ubuntu:24.04

RUN apt update && apt upgrade -y && \
    apt install -y --no-install-recommends \
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
    libsdl2-dev \
    libsdl2-image-dev \
    libsdl2-ttf-dev \
    libsdl2-mixer-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    mesa-utils \
    && apt clean && rm -rf /var/lib/apt/lists/*