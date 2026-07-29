# Builder: CUDA UR
FROM nvidia/cuda:13.3.0-cudnn-devel-ubuntu24.04 AS ur-builder

RUN apt-get update && apt-get install -y \
    git \
    cmake \
    ninja-build \
    libhwloc-dev \
    python3 \ 
    python3-pip \
    python3-venv

RUN git clone https://github.com/oneapi-src/unified-runtime.git

WORKDIR /unified-runtime

RUN python3 -m venv .venv
SHELL ["/bin/bash", "-c"]
RUN source .venv/bin/activate  \
    && pip install -r third_party/requirements.txt \
    && pip install lit filecheck \ 
    && cmake -B build -G Ninja -DUR_BUILD_ADAPTER_CUDA=true \
    && cmake --build build

# Builder: wacfrac
FROM intel/oneapi:2026.1.0-devel-ubuntu24.04 AS builder

ARG CUDA_VERSION=13-3
ARG SYCL_CUDA_ARCH=sm_80;sm_86;sm_89;sm_90;sm_120

ENV DEBIAN_FRONTEND=noninteractive

## Copy CUDA adapter
COPY --from=ur-builder /unified-runtime/build/lib/libur_adapter_cuda.so* /opt/intel/oneapi/compiler/latest/lib/
RUN find . -name "libur_adapter_cuda.so*"
ENV LD_LIBRARY_PATH=/opt/intel/oneapi/compiler/latest/lib:/opt/intel/oneapi/umf/1,1/lib:${LD_LIBRARY_PATH}

## Deps
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

## CUDA Toolkit (compiler)
RUN wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2404/x86_64/cuda-keyring_1.1-1_all.deb && \
    dpkg -i cuda-keyring_1.1-1_all.deb && \
    rm cuda-keyring_1.1-1_all.deb && \
    apt-get update && \
    apt-get install -y cuda-toolkit-${CUDA_VERSION};
ENV CUDA_PATH=/usr/local/cuda
ENV PATH=${CUDA_PATH}/bin:${PATH}
ENV LD_LIBRARY_PATH=${CUDA_PATH}/lib64:${LD_LIBRARY_PATH}

## Build
WORKDIR /app
COPY . .
RUN cmake -B build \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_C_COMPILER=icx \
        -DCMAKE_CXX_COMPILER=icpx \
        -DENABLE_CUDA_BACKEND=${INSTALL_CUDA} \
        -DSYCL_CUDA_ARCH=${SYCL_CUDA_ARCH} && \
    cmake --build build --parallel

# Runtime
FROM nvidia/cuda:12.8.0-runtime-ubuntu24.04

ENV DEBIAN_FRONTEND=noninteractive

# NOTE: Some of these libraries may be statically linked
RUN apt-get update && apt-get install -y --no-install-recommends \
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
    libmpc3 \
    && rm -rf /var/lib/apt/lists/*

## Copy Shared Objects
COPY --from=builder /opt/intel/oneapi/compiler/latest/lib /opt/intel/oneapi/compiler/latest/lib
COPY --from=builder /opt/intel/oneapi/umf/1.1/lib /opt/intel/oneapi/umf/1.1/lib
RUN ldconfig
    ENV LD_LIBRARY_PATH=/opt/intel/oneapi/compiler/latest/lib:/opt/intel/oneapi/umf/1.1/lib:${LD_LIBRARY_PATH}

ENV NVIDIA_VISIBLE_DEVICES=all
ENV NVIDIA_DRIVER_CAPABILITIES=compute,utility

COPY --from=builder /app/build/wacfrac /usr/local/bin/wacfrac
ENTRYPOINT ["wacfrac"]
