# ArcticEngine

Development Environment:

- Linux, Ubuntu 24.04
- VS Code using Dev Containers (Docker)
- CMake 4.3.2
- LLVM Clang 22.1.7
- C++ 23
- ClangD
- Clang Format
- Clang Tidy
- RenderDoc 1.44

Libraries used:

- [Vulkan](https://www.lunarg.com/vulkan-sdk/): a new generation graphics and compute API that provides high-efficiency, cross-platform access to modern graphics processing units (GPUs), `1.4.350.1`
- [SDL](https://github.com/libsdl-org/SDL): Simple Directmedia Layer (low level access to audio, keyboard, mouse, joystick, and graphics hardware),  `3.4.8`
- [GLM](https://github.com/g-truc/glm): OpenGL Mathematics (GLM), `1.0.3`
- [VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator): Easy to integrate Vulkan memory allocation library, `3.4.0`

# CMake Graph Visualizer

Use `graphviz` to visualize the dependency graph:

1. Navigate to your build directory
1. Run CMake with the --graphviz option
    - This will generate a graph.dot file in your build directory
1. Use dot (included in graphviz) to generate an image from the .dot file:
    - This will generate a graph.png image file in your build directory.

```sh
cd /path/to/your/build
cmake --graphviz=graph.dot .
dot -Tpng graph.dot -o graph.png
```
# Vulkan Test

To verify that vulkan works correctly,
run the following command:

```sh
vulkaninfo
```

and

```sh
vkvia
```

# Renderdoc

1. Open the terminal
1. Enter the following command to open renderdoc:
    ```bash
    qrenderdoc
    ```
1. Inside the app, a warning should tell you that vulkan needs to be configured for renderdoc
   Click on that warning, that enables you to generate the config. Make sure to generate in the user and not root.


# Build Shaders

`glslc` is a command-line tool that is part of the Shaderc project. It's essentially a compiler that transforms GLSL (OpenGL Shading Language) source code into SPIR-V binary format.

To use glsc, execute the following command format:

```bash
glslc <path_to_shader> -o <path_of_build>
```

For example:

```bash
glslc shader.vert -o vert.spv
glslc.exe shader.frag -o frag.spv
```

In these examples, `shader.vert` is the vertex shader and `shader.frag` is the fragment shader. The `-o` option specifies the output file, which will be in SPIR-V format.

Afterward, you can load these `.spv` files in your Vulkan application and create shader modules from them, which can then be used in the graphics pipeline.