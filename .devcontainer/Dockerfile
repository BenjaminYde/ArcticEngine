FROM ubuntu:24.04

RUN apt -qq update && \
    apt -qq install -y --no-install-recommends \
        autoconf \
        automake \
        build-essential \
        ca-certificates \
        curl \
        dos2unix \
        g++ \
        gcc \
        gdb \
        git \
        locales-all \
        make \
        nano \
        net-tools \
        ninja-build \
        python3 \
        rsync \
        ssh \
        sudo \
        tar \
        unzip \
        wget \
        zip \
        && \
    apt -qq clean && \
    apt -qq autoclean && \
    apt -qq remove && \
    apt -qq autoremove && \
    rm -rf /var/lib/apt/lists

# Install CMake
ENV CMAKE_VERSION 3.31.8
RUN apt update && \
    apt -qq install -y --no-install-recommends \
        graphviz \
    && \
    cd /tmp && \
    wget --no-check-certificate https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz && \
    tar -xzvf cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz && \
    mv cmake-${CMAKE_VERSION}-linux-x86_64 /opt/cmake && \
    rm cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz
ENV PATH="/opt/cmake/bin:${PATH}"

# Download and install Clang binaries
ENV CLANG_VERSION 20.1.7
RUN cd /tmp && \
    curl -L -O https://github.com/llvm/llvm-project/releases/download/llvmorg-${CLANG_VERSION}/LLVM-${CLANG_VERSION}-Linux-X64.tar.xz && \
    tar -xJvf LLVM-${CLANG_VERSION}-Linux-X64.tar.xz && \
    rm LLVM-${CLANG_VERSION}-Linux-X64.tar.xz

# Install Clang
RUN cd /tmp && \
    mkdir /opt/LLVM-${CLANG_VERSION} && \
    mv LLVM-${CLANG_VERSION}-Linux-X64/* /opt/LLVM-${CLANG_VERSION}
ENV PATH="/opt/LLVM-${CLANG_VERSION}/bin:${PATH}"

# Download and extract the LunarG Vulkan SDK
ENV VULKAN_SDK_VERSION 1.3.290.0
RUN cd /tmp && \
    curl -L -O https://sdk.lunarg.com/sdk/download/${VULKAN_SDK_VERSION}/linux/vulkansdk-linux-x86_64-${VULKAN_SDK_VERSION}.tar.xz && \
    tar xJf vulkansdk-linux-x86_64-${VULKAN_SDK_VERSION}.tar.xz && \
    rm vulkansdk-linux-x86_64-${VULKAN_SDK_VERSION}.tar.xz && \
    mv ${VULKAN_SDK_VERSION} /opt/vulkansdk-${VULKAN_SDK_VERSION}
    
# Setup Vulkan SDK environment variables
ENV VULKAN_SDK="/opt/vulkansdk-${VULKAN_SDK_VERSION}/x86_64"
ENV PATH="${VULKAN_SDK}/bin:$PATH"
ENV LD_LIBRARY_PATH="${VULKAN_SDK}/lib:$LD_LIBRARY_PATH"
ENV VK_LAYER_PATH="${VULKAN_SDK}/etc/vulkan/explicit_layer.d"

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
        "api_version": "1.4.313"
    }
}
EOF

# Install APT dependencies for rendering
RUN apt-get update && apt-get install -y --install-recommends \
    # vulkan
    libvulkan1 \
    mesa-vulkan-drivers \
    vulkan-tools \
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

# Download SDL source code
ENV SDL_VERSION 2.32.8
RUN cd /tmp && \
    wget --no-check-certificate https://github.com/libsdl-org/SDL/archive/refs/tags/release-${SDL_VERSION}.tar.gz -O SDL-${SDL_VERSION}.tar.gz && \
    tar -xzvf SDL-${SDL_VERSION}.tar.gz && \
    rm SDL-${SDL_VERSION}.tar.gz 

# Build and install SDL from source
RUN cd /tmp/SDL-release-${SDL_VERSION} && \
    # build
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build && \
    # install
    mkdir -p /opt/SDL-${SDL_VERSION} && \
    cmake --install build --prefix /opt/SDL-${SDL_VERSION}
ENV CMAKE_PREFIX_PATH="/opt/SDL-${SDL_VERSION}:${CMAKE_PREFIX_PATH}"

# Install RenderDoc
ENV RENDERDOC_VERSION 1.39
RUN cd /tmp && \
    curl -L -O https://renderdoc.org/stable/${RENDERDOC_VERSION}/renderdoc_${RENDERDOC_VERSION}.tar.gz \
    && tar -xzf renderdoc_${RENDERDOC_VERSION}.tar.gz -C /opt \
    && rm renderdoc_${RENDERDOC_VERSION}.tar.gz
ENV PATH="/opt/renderdoc_${RENDERDOC_VERSION}/bin:${PATH}"
RUN sudo apt -qq install -y --no-install-recommends libproxy1v5

# Install RenderDoc extensions
RUN cd /tmp && \
    curl -L -O https://renderdoc.org/stable/${RENDERDOC_VERSION}/renderdoc_${RENDERDOC_VERSION}.tar.gz \
    && tar -xzf renderdoc_${RENDERDOC_VERSION}.tar.gz -C /opt \
    && rm renderdoc_${RENDERDOC_VERSION}.tar.gz

# Create User
ARG USER_UID=1000
RUN deluser --remove-home ubuntu && \
    useradd --create-home --user-group --groups sudo --uid ${USER_UID} developer && \
    chown -R developer:developer /home/developer
ENV USER=developer
WORKDIR /home/developer

# Install ZSH
RUN sudo apt update && sudo apt -qq install -y --no-install-recommends \
        zsh \
        && \
    sh -c "$(curl -fsSL https://raw.github.com/ohmyzsh/ohmyzsh/master/tools/install.sh)" && \
    touch .zshrc && \
    echo '\n# ZSH' >> .zshrc && \
    echo "export ZSH=\"/home/developer/.oh-my-zsh\"" >> .zshrc && \
    echo "export ZSH_THEME=\"eastwood\"" >> .zshrc && \
    echo "plugins=(git)" >> .zshrc && \
    echo "source /home/developer/.oh-my-zsh/oh-my-zsh.sh" >> .zshrc

ENTRYPOINT ["/usr/bin/zsh"]