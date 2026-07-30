# Builder
FROM nvidia/cuda:12.8.1-devel-ubuntu24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

## Download SYCL binaries
ENV DPCPP_DIR=/opt/intel/dpcpp
RUN mkdir -p $DPCPP_DIR
WORKDIR $DPCPP_DIR

## Install all Deps
RUN apt-get update && apt-get install -y --no-install-recommends \
    wget \
    g++-14 \
    cmake \
    ninja-build \
    git \
    pkg-config \
    curl \
    zip \
    unzip \
    tar \
    ffmpeg \
    libboost-all-dev \
    libgmp-dev \
    libmpfr-dev \
    libmpc-dev \
    && rm -rf /var/lib/apt/lists/*

### Download SYCL Compiler release
RUN wget https://github.com/intel/llvm/releases/download/v7.0.0/sycl_linux.tar.gz
RUN tar -xvf sycl_linux.tar.gz
RUN rm sycl_linux.tar.gz

### Update ENV
WORKDIR /
ENV PATH=$DPCPP_DIR/bin:$PATH
ENV LD_LIBRARY_PATH=$DPCPP_DIR/lib:$LD_LIBRARY_PATH

## Build
ARG BUILD_TYPE=Release
COPY . /app
WORKDIR /app
RUN cmake -B build \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
        -DCMAKE_C_COMPILER=clang \
        -DCMAKE_CXX_COMPILER=clang++ \
        -DENABLE_CUDA_BACKEND=ON \
        -DSYCL_CUDA_ARCH=${SYCL_CUDA_ARCH} && \
    cmake --build build --parallel

# Runtime
FROM nvidia/cuda:12.8.0-runtime-ubuntu24.04

ENV DEBIAN_FRONTEND=noninteractive

ENV SYCL_CACHE_PERSISTENT=1

## Install deps
RUN apt-get update && apt-get install -y --no-install-recommends \
    wget \
    ocl-icd-libopencl1 \ 
    intel-opencl-icd \
    clinfo \ 
    ffmpeg \
    libhwloc-dev \
    libboost-log1.83.0 \ 
    libboost-thread1.83.0 \
    libboost-filesystem1.83.0 \
    libboost-chrono1.83.0 \
    libboost-atomic1.83.0 \
    libboost-regex1.83.0 \
    libboost-container1.83.0 \
    libboost-date-time1.83.0 \
    libboost-serialization1.83.0 \
    libgmp10 \
    libmpfr6 \
    libmpc3

## Install OpenCL runtime
RUN mkdir /tmp/neo && cd /tmp/neo && \
    wget https://github.com/intel/intel-graphics-compiler/releases/download/igc-1.0.17193.4/intel-igc-core_1.0.17193.4_amd64.deb && \
    wget https://github.com/intel/intel-graphics-compiler/releases/download/igc-1.0.17193.4/intel-igc-opencl_1.0.17193.4_amd64.deb && \
    wget https://github.com/intel/compute-runtime/releases/download/24.26.30049.6/intel-level-zero-gpu_1.3.30049.6_amd64.deb && \
    wget https://github.com/intel/compute-runtime/releases/download/24.26.30049.6/intel-opencl-icd_24.26.30049.6_amd64.deb && \
    wget https://github.com/intel/compute-runtime/releases/download/24.26.30049.6/libigdgmm12_22.3.20_amd64.deb && \
    dpkg -i *.deb && rm -rf /tmp/neo

## Copy binary and needed libraries
COPY ./examples/ ~/
COPY --from=builder /opt/intel/dpcpp/lib /usr/local/lib/
COPY --from=builder /app/build/wacfrac /usr/local/bin/wacfrac
ENV LD_LIBRARY_PATH=${LD_LIBRARY_PATH}
RUN ldconfig

WORKDIR /root
ENTRYPOINT ["wacfrac"]
