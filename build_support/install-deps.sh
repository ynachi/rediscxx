#!/usr/bin/env bash

git clone https://github.com/scylladb/seastar.git && export DEBIAN_FRONTEND=noninteractive  && cd seastar \
    && sudo apt-get update && sudo apt-get install -y \
    curl \
    gnupg \
    && echo "deb http://apt.llvm.org/noble/ llvm-toolchain-noble-18 main" \
    | sudo tee -a /etc/apt/sources.list.d/llvm.list \
    && echo "deb http://apt.llvm.org/noble/ llvm-toolchain-noble-19 main" \
    | sudo tee -a /etc/apt/sources.list.d/llvm.list \
    && sudo curl -sSL https://apt.llvm.org/llvm-snapshot.gpg.key -o /etc/apt/trusted.gpg.d/apt.llvm.org.asc \
    && sudo apt-get update && sudo apt-get install -y \
    build-essential \
    clang-18 \
    clang-19 \
    clang-tools-19 \
    gcc-13 \
    g++-13 \
    gcc-14 \
    g++-14 \
    pandoc \
    && sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 13 \
    && sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 13 \
    && sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-14 14 \
    && sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-14 14 \
    && sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-18 18 \
    && sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-19 19 \
    && sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-19 19 \
    && sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-19 19 \
    && sudo bash ./install-dependencies.sh \
    && sudo apt-get clean \
    && sudo rm -rf /var/lib/apt/lists/* && rm -rf seastar
