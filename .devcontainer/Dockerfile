FROM ubuntu:24.04

# Development APT packages
RUN apt -qq update && \
    apt -qq install -y --no-install-recommends \
        sudo \
        git \
        wget \
        curl \
        zip \
        unzip \
        tar \
        ca-certificates \
        build-essential \
        gcc \
        g++ \
        gdb \
        make \
        ninja-build \
        autoconf \
        automake \
        locales-all \
        dos2unix \
        rsync \
        tar \
        ssh \
        python3 \
        && \
    apt -qq clean && \
    apt -qq autoclean && \
    apt -qq remove && \
    apt -qq autoremove

# Install CMake
ENV CMAKE_VERSION 3.31.2
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
ENV CLANG_VERSION 19.1.5
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
#ENV VK_ICD_FILENAMES /usr/share/vulkan/icd.d/nvidia_icd.json
ENV VK_ICD_FILENAMES=/vulkan-icd/nvidia_icd.json

RUN ICD_NVIDIA="/vulkan-icd/nvidia_icd.json" && \
    mkdir -p /vulkan-icd && \
    touch /vulkan-icd/nvidia_icd.json \
    && echo '{' >> $ICD_NVIDIA \
    && echo '    "file_format_version" : "1.0.0",' >> $ICD_NVIDIA \
    && echo '    "ICD": {' >> $ICD_NVIDIA \
    && echo '        "library_path": "libGLX_nvidia.so.0",' >> $ICD_NVIDIA \
    && echo '        "api_version" : "1.3.290"' >> $ICD_NVIDIA \
    && echo '    }' >> $ICD_NVIDIA \
    && echo '}' >> /$ICD_NVIDIA

# Install vulkan dependencies
RUN apt-get update && apt-get install -y --install-recommends \
    #other
    nano \
    net-tools \
    # vulkan
    libvulkan1 \
    mesa-vulkan-drivers \
    vulkan-tools

# Install XServer dependencies
RUN apt-get update && apt-get install -y --install-recommends \
    # open gl
    libegl1-mesa-dev \    
    libgl1-mesa-dev \
    libgles2-mesa-dev \
    # x general
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    xauth \
    libx11-dev \
    libx11-xcb-dev \
    libxi-dev \
    libxcb1-dev \
    libxcb-randr0-dev \
    libxfixes-dev \
    libxss-dev \
    libxkbcommon-dev

# Install SDL2 (no source)
#RUN apt-get update && apt-get install -y --no-install-recommends \
#    # SDL2
#    libsdl2-dev \
#    libsdl2-image-dev \
#    libsdl2-mixer-dev \
#    libsdl2-net-dev \
#    libsdl2-ttf-dev

# Install SDL2 dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    libfreeimage-dev \
    libopenal-dev \
    libpango1.0-dev \
    libsndfile-dev \
    libudev-dev \
    libasound2-dev \
    libjpeg-dev \
    libtiff5-dev \
    libwebp-dev \
    libaudio-dev \
    libpulse-dev \ 
    libjack-dev \
    libxext-dev \
    libdbus-1-dev \
    libibus-1.0-dev \
    fcitx-libs-dev \ 
    libdrm-dev \
    libgbm-dev \
    libpipewire-0.3-dev \
    libwayland-dev \
    libdecor-0-dev

# Download and install SDL2 from source
ENV SDL_VERSION 2.30.8
#RUN cd /tmp && \
#    curl -L -O https://github.com/libsdl-org/SDL/archive/refs/tags/release-${SDL2_VERSION}.tar.gz && \
#    tar xzf SDL2-${SDL2_VERSION}.tar.gz && \
#    rm SDL2-${SDL2_VERSION}.tar.gz && \
#    cd SDL2-${SDL2_VERSION} && \
#    ./configure && \
#    make && \
#    make install

# Download SDL source code
RUN cd /tmp && \
    wget --no-check-certificate https://github.com/libsdl-org/SDL/releases/download/release-${SDL_VERSION}/SDL2-${SDL_VERSION}.tar.gz && \
    tar -xzvf SDL2-${SDL_VERSION}.tar.gz && \
    rm SDL2-${SDL_VERSION}.tar.gz 

# Build and install SDL2 from source
RUN cd /tmp/SDL2-${SDL_VERSION} && \
    mkdir /opt/SDL2-${SDL_VERSION} && \
    #./configure && \
    cmake -S . -B /opt/SDL2-${SDL_VERSION} -DCMAKE_BUILD_TYPE=Release && \
    cmake --build /opt/SDL2-${SDL_VERSION} && \
    #cd /opt/SDL2-${SDL_VERSION} && \
    #make && \
    cmake --install /opt/SDL2-${SDL_VERSION}
#ENV CMAKE_PREFIX_PATH="/opt/SDL2-${SDL_VERSION}:${CMAKE_PREFIX_PATH}"

# Install RenderDoc
ENV RENDERDOC_VERSION 1.35
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
RUN deluser --remove-home ubuntu && \
    useradd --create-home --user-group --groups sudo --uid 1000 developer && \
    chown -R developer:developer /home/developer && \
    printf "\developer ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers
ENV USER=developer \
    PATH=/home/developer/.local/bin:$PATH
USER developer
WORKDIR /home/developer

# Install ZSH
RUN sudo apt update && sudo apt -qq install -y --no-install-recommends \
        zsh \
        && \
    sh -c "$(curl -fsSL https://raw.github.com/ohmyzsh/ohmyzsh/master/tools/install.sh)" && \
    rm .zshrc && \
    touch .zshrc && \
    echo '\n# ZSH' >> .zshrc && \
    echo "export ZSH=\"/home/developer/.oh-my-zsh\"" >> .zshrc && \
    echo "export ZSH_THEME=\"eastwood\"" >> .zshrc && \
    echo "plugins=(git)" >> .zshrc && \
    echo "source /home/developer/.oh-my-zsh/oh-my-zsh.sh" >> .zshrc

ENTRYPOINT ["/usr/bin/zsh"]