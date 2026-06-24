# UBUNTU BASE
FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive
ARG DEVELOPER_UID=1000
LABEL Author="BenjaminYde"

# -------------------------------------------------------------------------
# Enable APT cache persistence for BuildKit cache mounts
# -------------------------------------------------------------------------
#
# Docker's official Ubuntu images ship with /etc/apt/apt.conf.d/docker-clean,
# a hook that deletes all downloaded .deb files from /var/cache/apt after every
# apt-get install. This is good for keeping image layers small in normal builds,
# but it defeats the purpose of --mount=type=cache,target=/var/cache/apt:
# the cache mount would persist an empty directory every time.
#
# What we do here:
#   1. Remove docker-clean           → stops apt from wiping .deb files
#   2. Set Keep-Downloaded-Packages  → explicitly tells apt to retain them
#
# After this, the two cache mounts used throughout this Dockerfile work as
# intended:
#   /var/lib/apt/lists  — caches the package index  (works without this fix)
#   /var/cache/apt      — caches downloaded .deb's  (NEEDS this fix)
#
# The result: only new or updated packages are downloaded on rebuild.
RUN rm -f /etc/apt/apt.conf.d/docker-clean && \
    echo 'Binary::apt::APT::Keep-Downloaded-Packages "true";' \
    > /etc/apt/apt.conf.d/keep-cache

# ---------------------------------------------------------------------------
# Install APT packages
# ---------------------------------------------------------------------------
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt -qq update && \
    apt -qq install -y --no-install-recommends \
        # general
        apt-utils \
        lsb-release \
        # git
        git \
        git-lfs \
        # files & io
        parallel \
        rsync \
        unzip \
        tree \
        # networking & remote
        curl \
        openssh-client \
        ssh \
        wget \
        xdg-utils \
        # cpp
        build-essential \
        ccache \
        autoconf \
        automake \
        dos2unix \
        g++ \
        gcc \
        gdb \
        make \
        ninja-build \
        software-properties-common \
        # permissions
        sudo \
        gnupg \
        ca-certificates \
        # terminal
        bash-completion \
        nano \
        zsh

# ---------------------------------------------------------------------------
# Install CMake - cross-platform build system generator
# ---------------------------------------------------------------------------
ENV CMAKE_VERSION 4.3.3
RUN \
    # Install CMake
    cd /tmp && \
    wget --no-check-certificate https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz && \
    tar -xzvf cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz && \
    mv cmake-${CMAKE_VERSION}-linux-x86_64 /opt/cmake && \
    rm cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz && \
    # Install CMake Tools
    apt update && \
    apt -qq install -y --no-install-recommends \
        graphviz
ENV PATH="/opt/cmake/bin:${PATH}"

# ---------------------------------------------------------------------------
# Install Clang - C/C++ compiler, linker and tools (clangd, clang-format, clang-tidy)
# ---------------------------------------------------------------------------
# Install Clang APT Packages
# Checkout the website: https://llvm.org/
# Checkout the website: https://apt.llvm.org/
# Checkout the website: https://apt.llvm.org/noble/pool/main/l/llvm-toolchain-22/
ENV CLANG_MAJOR_VERSION="22"
RUN \
    # Download and add the official LLVM GPG key
    wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - && \
    # # Add the LLVM repository
    echo "deb http://apt.llvm.org/$(lsb_release -sc)/ llvm-toolchain-$(lsb_release -sc)-${CLANG_MAJOR_VERSION} main" > /etc/apt/sources.list.d/llvm-${CLANG_MAJOR_VERSION}.list && \
    # Install APT packages
    # - clang-XX: The C/C++ compiler
    # - lld-XX: The fast linker
    # - libc++-XX-dev: The C++ standard library headers
    # - libc++abi-XX-dev: The C++ ABI library headers
    apt update && apt install -y --no-install-recommends \
        clang-${CLANG_MAJOR_VERSION} \
        lld-${CLANG_MAJOR_VERSION} \
        libc++-${CLANG_MAJOR_VERSION}-dev \
        libc++abi-${CLANG_MAJOR_VERSION}-dev \
        clang-tools-${CLANG_MAJOR_VERSION} \
        clang-${CLANG_MAJOR_VERSION}-doc \
        libclang-common-${CLANG_MAJOR_VERSION}-dev \
        libclang-${CLANG_MAJOR_VERSION}-dev \
        clang-format-${CLANG_MAJOR_VERSION}  \
        libclang1-${CLANG_MAJOR_VERSION} \
        clangd-${CLANG_MAJOR_VERSION} \
        clang-tidy-${CLANG_MAJOR_VERSION} \
    && \
    # Create symlinks so you can just run `clang++`
ln -s /usr/bin/clang-${CLANG_MAJOR_VERSION} /usr/bin/clang && \
    ln -s /usr/bin/clang++-${CLANG_MAJOR_VERSION} /usr/bin/clang++ && \
    ln -s /usr/bin/clangd-${CLANG_MAJOR_VERSION} /usr/bin/clangd && \
    ln -s /usr/bin/clang-format-${CLANG_MAJOR_VERSION} /usr/bin/clang-format && \
    ln -s /usr/bin/clang-tidy-${CLANG_MAJOR_VERSION} /usr/bin/clang-tidy

# ---------------------------------------------------------------------------
# LunarG Vulkan SDK
# ---------------------------------------------------------------------------
ENV VULKAN_SDK_VERSION 1.4.350.1
RUN cd /tmp && \
    curl -L -O https://sdk.lunarg.com/sdk/download/${VULKAN_SDK_VERSION}/linux/vulkansdk-linux-x86_64-${VULKAN_SDK_VERSION}.tar.xz && \
    tar xJf vulkansdk-linux-x86_64-${VULKAN_SDK_VERSION}.tar.xz && \
    rm vulkansdk-linux-x86_64-${VULKAN_SDK_VERSION}.tar.xz && \
    mv ${VULKAN_SDK_VERSION} /opt/vulkansdk-${VULKAN_SDK_VERSION}

# Setup Vulkan SDK environment variables
ENV VULKAN_SDK="/opt/vulkansdk-${VULKAN_SDK_VERSION}/x86_64"
ENV PATH="${VULKAN_SDK}/bin:$PATH"
ENV LD_LIBRARY_PATH="${VULKAN_SDK}/lib:$LD_LIBRARY_PATH"
ENV VK_LAYER_PATH="${VULKAN_SDK}/share/vulkan/explicit_layer.d"

# Set environment variables for NVIDIA and Vulkan
ENV NVIDIA_VISIBLE_DEVICES all
ENV NVIDIA_DRIVER_CAPABILITIES graphics,utility,compute
ENV VK_ICD_FILENAMES=/opt/vulkan-icd/nvidia_icd.json
RUN mkdir -p $(dirname ${VK_ICD_FILENAMES}) && \
    # use a heredoc for the JSON content for readability
    cat <<EOF > ${VK_ICD_FILENAMES}
{
    "file_format_version": "1.0.0",
    "ICD": {
        "library_path": "libGLX_nvidia.so.0",
        "api_version": "1.4.350"
    }
}
EOF

# X11 / rendering system libraries
# ---------------------------------------------------------------------------
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get -qq update && \
    apt-get -qq install -y --no-install-recommends \
    # vulkan
    libvulkan1 \
    mesa-vulkan-drivers \
    vulkan-tools \
    libvulkan-dev \
    # open gl
    libegl1-mesa-dev \
    libgl1-mesa-dev \
    libgles2-mesa-dev \
    # x11 general
    libx11-dev \
    libx11-xcb-dev \
    libxcb-randr0-dev \
    libxcb1-dev \
    libxcursor-dev \
    libxfixes-dev \
    libxi-dev \
    libxinerama-dev \
    libxkbcommon-dev \
    libxrandr-dev \
    libxss-dev \
    xauth \
    # sdl
    fcitx-libs-dev \
    libasound2-dev \
    libaudio-dev \
    libdbus-1-dev \
    libdecor-0-dev \
    libdrm-dev \
    libfreeimage-dev \
    libgbm-dev \
    libibus-1.0-dev \
    libjack-dev \
    libjpeg-dev \
    libopenal-dev \
    libpango1.0-dev \
    libpipewire-0.3-dev \
    libpulse-dev \
    libsndfile-dev \
    libtiff5-dev \
    libudev-dev \
    libwayland-dev \
    libwebp-dev \
    libxext-dev

# ---------------------------------------------------------------------------
# SDL3 — windowing and input
# ---------------------------------------------------------------------------
ENV SDL_VERSION=3.4.8
ENV SDL_DIR=/opt/sdl3-${SDL_VERSION}

RUN cd /tmp && \
    # download
    wget -q \
        https://github.com/libsdl-org/SDL/archive/refs/tags/release-${SDL_VERSION}.tar.gz \
        -O SDL-${SDL_VERSION}.tar.gz && \
    tar -xzf SDL-${SDL_VERSION}.tar.gz && \
    rm SDL-${SDL_VERSION}.tar.gz && \
    # cmake configure
    cmake \
        -S SDL-release-${SDL_VERSION} \
        -B SDL-release-${SDL_VERSION}/build \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_C_COMPILER=clang-22 \
        -DCMAKE_CXX_COMPILER=clang++-22 \
        -DCMAKE_INSTALL_PREFIX=${SDL_DIR} \
        -DSDL_SHARED=ON \
        -DSDL_STATIC=ON \
        -DSDL_VULKAN=ON \
        -DSDL_WAYLAND=ON \
        -DSDL_X11=ON \
        -DSDL_X11_XTEST=OFF \
        -DSDL_TEST_LIBRARY=OFF && \
    # cmake build
    cmake --build   SDL-release-${SDL_VERSION}/build -j$(nproc) && \
    # cmake install
    cmake --install SDL-release-${SDL_VERSION}/build && \
    # cleanups
    rm -rf /tmp/SDL-release-${SDL_VERSION}

ENV CMAKE_PREFIX_PATH="${SDL_DIR}:${CMAKE_PREFIX_PATH}"
ENV PKG_CONFIG_PATH="${SDL_DIR}/lib/pkgconfig:${SDL_DIR}/lib64/pkgconfig:${PKG_CONFIG_PATH}"
ENV LD_LIBRARY_PATH="${SDL_DIR}/lib:${SDL_DIR}/lib64:${LD_LIBRARY_PATH}"

# ---------------------------------------------------------------------------
# RenderDoc — graphics debugger
# ---------------------------------------------------------------------------
ENV RENDERDOC_VERSION 1.44
ENV RENDERDOC_DIR=/opt/renderdoc-${RENDERDOC_VERSION}
RUN cd /tmp && \
    # download and install
    curl -fsSL -O \
        https://renderdoc.org/stable/${RENDERDOC_VERSION}/renderdoc_${RENDERDOC_VERSION}.tar.gz && \
    tar -xzf renderdoc_${RENDERDOC_VERSION}.tar.gz -C /opt && \
    mv /opt/renderdoc_${RENDERDOC_VERSION} ${RENDERDOC_DIR} && \
    rm renderdoc_${RENDERDOC_VERSION}.tar.gz && \
    # dependencies
    apt-get -qq update && \
    apt-get -qq install -y --no-install-recommends libproxy1v5 && \
    # cleanups
    apt-get -qq clean && rm -rf /var/lib/apt/lists/*

# ---------------------------------------------------------------------------
# Ubuntu User
# ---------------------------------------------------------------------------
ARG DEVELOPER_UID=1000
RUN deluser --remove-home ubuntu && \
useradd --create-home --user-group --groups sudo --uid ${DEVELOPER_UID} developer && \
printf "\ndeveloper ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers
USER developer
ENV USER="developer" HOME="/home/developer"
WORKDIR $HOME

# ---------------------------------------------------------------------------
# ZSH + Oh My Zsh - terminal and shell configuration framework
# ---------------------------------------------------------------------------
RUN sudo apt install -y zsh && \
    sudo chsh -s /usr/bin/zsh && \
    # Install oh my zsh
    sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)" && \
    # configure .zshrc
    cd ~ && \
    touch .zshrc && \
    sed -i 's/ZSH_THEME=.*/ZSH_THEME="eastwood"/' .zshrc && \
    echo "ZSH Installed"

ENTRYPOINT ["/usr/bin/zsh"]