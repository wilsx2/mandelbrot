# Builder
FROM ubuntu:24.04 AS builder

ARG INSTALL_CUDA=false
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends wget gpg ca-certificates \
    && wget -O- https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB \
       | gpg --dearmor | tee /usr/share/keyrings/oneapi-archive-keyring.gpg > /dev/null \
    && echo "deb [signed-by=/usr/share/keyrings/oneapi-archive-keyring.gpg] https://apt.repos.intel.com/oneapi all main" \
       | tee /etc/apt/sources.list.d/oneAPI.list \
    && apt-get update

RUN apt-get install -y intel-oneapi-compiler-dpcpp-cpp \
    && apt-get install -y --no-install-recommends \
    g++-14 \
    cmake \
    ninja-build \
    git \
    pkg-config \
    libgmp-dev \
    libmpfr-dev \
    libmpc-dev \
    libboost-all-dev \
    libtbb-dev \
    curl \
    zip \
    unzip \
    tar \
    && rm -rf /var/lib/apt/lists/*

# Install NVIDIA CUDA toolkit (optional)
RUN if [ "$INSTALL_CUDA" = "true" ]; then \
        wget -q https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2404/x86_64/cuda-keyring_1.1-1_all.deb \
        && dpkg -i cuda-keyring_1.1-1_all.deb \
        && apt-get update \
        && apt-get install -y --no-install-recommends cuda-toolkit-12-6 \
        && rm -rf /var/lib/apt/lists/* \
        && rm cuda-keyring_1.1-1_all.deb; \
    fi

ENV PATH=/usr/local/cuda/bin:${PATH} \
    LD_LIBRARY_PATH=/usr/local/cuda/lib64:${LD_LIBRARY_PATH} \
    CUDA_PATH=/usr/local/cuda

WORKDIR /app
COPY . .

RUN . /opt/intel/oneapi/setvars.sh \
    && SYCL_TARGETS="spir64" \
    && if [ "$INSTALL_CUDA" = "true" ]; then SYCL_TARGETS="${SYCL_TARGETS},nvptx64-nvidia-cuda"; fi \
    && cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=/opt/intel/oneapi/compiler/latest/bin/icx \
    -DCMAKE_CXX_COMPILER=/opt/intel/oneapi/compiler/latest/bin/icpx \
    -DCMAKE_CXX_FLAGS="-fsycl -fsycl-targets=${SYCL_TARGETS} -w" \
    && cmake --build build --parallel

RUN mkdir -p /usr/local/cuda/lib64

# Runtime
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends wget gpg ca-certificates \
    && wget -O- https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB \
       | gpg --dearmor | tee /usr/share/keyrings/oneapi-archive-keyring.gpg > /dev/null \
    && echo "deb [signed-by=/usr/share/keyrings/oneapi-archive-keyring.gpg] https://apt.repos.intel.com/oneapi all main" \
       | tee /etc/apt/sources.list.d/oneAPI.list \
    && apt-get update

RUN apt-get update && apt-get install -y --no-install-recommends \
    intel-oneapi-runtime-dpcpp-cpp \
    intel-oneapi-runtime-opencl \
    intel-opencl-icd \
    libze-intel-gpu1 \
    libze1 \
    libgmp10 \
    libmpfr6 \
    libmpc3 \
    libstdc++6 \
    ffmpeg \
    libboost-log1.83.0 \
    libboost-chrono1.83.0 \
    libboost-thread1.83.0 \
    libboost-filesystem1.83.0 \
    libboost-atomic1.83.0 \
    libboost-regex1.83.0 \
    libboost-date-time1.83.0 \
    libboost-container1.83.0 \
    libboost-serialization1.83.0 \
    && rm -rf /var/lib/apt/lists/*

RUN wget -q "https://github.com/oneapi-src/level-zero/releases/download/v1.28.2/level-zero_1.28.2+u24.04_amd64.deb" \
    && dpkg -i --force-overwrite level-zero_1.28.2+u24.04_amd64.deb \
    && rm level-zero_1.28.2+u24.04_amd64.deb

# CUDA only (optional)
ENV LD_LIBRARY_PATH=/usr/local/cuda/lib64:${LD_LIBRARY_PATH}
ENV NVIDIA_VISIBLE_DEVICES=all \
    NVIDIA_DRIVER_CAPABILITIES=compute,utility
COPY --from=builder /usr/local/cuda/lib64/ /usr/local/cuda/lib64/

COPY --from=builder /opt/intel/oneapi/umf/1.1/lib/ /opt/intel/oneapi/redist/lib/
COPY --from=builder /app/build/wacfrac /usr/local/bin/wacfrac

ENTRYPOINT ["wacfrac"]
