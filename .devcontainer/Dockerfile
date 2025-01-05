FROM ubuntu:25.04

RUN apt update && apt upgrade -y && apt install -y \
    g++ \
    cmake \
    ninja-build \
    git \
    bash \
    vim \
    && apt clean && rm -rf /var/lib/apt/lists/*