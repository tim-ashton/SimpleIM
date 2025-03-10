FROM ubuntu:24.04

RUN apt update && apt upgrade -y && apt install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    bash \
    vim \
    && apt clean && rm -rf /var/lib/apt/lists/*