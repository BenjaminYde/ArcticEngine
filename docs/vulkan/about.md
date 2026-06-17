# About

## What is Vulkan?

Vulkan is a modern, cross-platform, low-level graphics and compute API  It was released in 2016 as a successor to OpenGL, aiming to provide more efficient and flexible access to GPU resources for developers. Vulkan is designed for high-performance 3D graphics, as well as general-purpose computing tasks on the GPU.

## Khronos Group

The Khronos Group is an open standards consortium founded in 2000, consisting of various companies and organizations from the technology industry, including major hardware manufacturers, software developers, and academic institutions. The group's primary focus is on creating and maintaining open, royalty-free standards for graphics, parallel computing, virtual and augmented reality, and other related domains.

Even if you haven’t heard of Khronos, you’ve probably heard of some of their standards, such as: OpenGL, OpenGL ES, WebGL, OpenCL, SPIR, SYCL, WebCL, OpenVX, EGL, OpenMAX, OpenVG, OpenSL ES, StreamInput, COLLADA, and glTF.

The development of Vulkan within the Khronos Group follows the organization's commitment to open standards, ensuring that the API remains vendor-neutral and widely accessible. This approach has led to Vulkan's support on various platforms and its adoption by multiple GPU manufacturers, such as AMD, NVIDIA, Intel, and others.

## Why Vulkan Exists

Older graphics APIs, like OpenGL, were designed in the 1990s when computers had only one CPU core and graphics cards worked very differently. As GPUs became incredibly powerful parallel processors and CPUs gained multiple cores, older APIs became a bottleneck. Vulkan was built from the ground up to fix this. It gives developers explicit, low-level control over the hardware, drastically lowering CPU overhead and allowing multiple CPU cores to feed data to the GPU at the exact same time.

## Origin of Vulkan

Vulkan has its origins in AMD's (Advanced Micro Devices) Mantle API, which was a low-level, high-performance graphics API developed in collaboration with game developers like DICE. Mantle was introduced in 2013, and its primary goal was to reduce driver overhead and improve the performance of games on AMD hardware. Mantle demonstrated the potential for low-level APIs to provide significant performance improvements over traditional APIs like OpenGL and DirectX 11.

In 2014, AMD announced that it would be contributing Mantle to the Khronos Group, an open standards consortium responsible for the development of APIs such as OpenGL, OpenCL, and WebGL. The goal was to create a new, cross-vendor, cross-platform graphics API based on Mantle's principles. This decision led to the development of Vulkan, which aimed to provide a low-level, high-performance graphics API that could be used across various platforms and GPU vendors.

Vulkan was officially announced by the Khronos Group on March 3, 2015, under the codename "GLnext" (indicating its role as the successor to OpenGL). The first official version, Vulkan 1.0, was released on February 16, 2016.

The development of Vulkan was driven by the industry's need for a more efficient and flexible graphics API that could better leverage modern hardware. Vulkan addressed this need by providing developers with more direct control over GPU resources and enabling better multi-threading capabilities. This allowed for significant performance improvements and better utilization of multi-core processors compared to traditional APIs like OpenGL and DirectX 11.

As graphics card architectures matured, they started offering more and more programmable functionality. All this new functionality had to be integrated with the existing APIs somehow. This resulted in less than ideal abstractions and a lot of guesswork on the graphics driver side to map the programmer's intent to the modern graphics architectures. That's why there are so many driver updates for improving the performance in games, sometimes by significant margins. Because of the complexity of these drivers, application developers also need to deal with inconsistencies between vendors. Aside from these new features, the past decade also saw an influx of mobile devices with powerful graphics hardware. These mobile GPUs have different architectures based on their energy and space requirements.

## Graphics API Comparison

Here's a comparison between Vulkan, OpenGL, and DirectX:

### 1. Platform Support

- **Vulkan**: It is a cross-platform API, supporting Windows, Linux, Android, macOS, and iOS (via the MoltenVK library).

- **OpenGL**: Also a cross-platform API, it supports Windows, Linux, macOS, and mobile platforms such as Android and iOS.

- **DirectX**: Developed by Microsoft, it is primarily designed for the Windows ecosystem, including Xbox consoles. DirectX 12, the latest version, is available on Windows 10 and Xbox One.

### 2. Performance and Low-Level Control

- **Vulkan**: Offers low-level control over GPU resources, enabling developers to optimize their applications better, reducing driver overhead, and facilitating multi-threaded rendering. This leads to better performance, especially on modern hardware.

- **OpenGL**: A higher-level API compared to Vulkan, it doesn't provide the same level of control over GPU resources, resulting in higher driver overhead and less efficient multi-threaded rendering.

- **DirectX**: DirectX 12, like Vulkan, offers low-level control and is designed for high-performance graphics. However, older versions of DirectX (e.g., DirectX 11) are more comparable to OpenGL in terms of abstraction and performance characteristics.

### 3. Learning Curve and Ease of Use

- **Vulkan**: Due to its low-level nature, Vulkan has a steeper learning curve and can be more challenging to work with compared to OpenGL and older versions of DirectX.

- **OpenGL**: As a higher-level API, OpenGL is considered easier to learn and use, making it a popular choice for beginners and educational purposes.

- **DirectX**: DirectX 12's low-level nature makes it more challenging to work with, similar to Vulkan. However, older versions (e.g., DirectX 11) are considered easier to learn.

### 4. Community and Ecosystem

- **Vulkan**: As a relatively new API, its community and ecosystem are still growing. However, many game engines and middleware solutions have started adopting Vulkan due to its performance advantages.

- **OpenGL**: Boasts a large and established community, with a wealth of resources, tutorials, and tools available. However, its relevance has diminished in recent years due to the rise of Vulkan and DirectX 12.

- **DirectX**: A widely used API, particularly in the gaming industry, DirectX has a strong community and extensive support from both Microsoft and third-party developers.

## Vulkan Validation Layers

The Vulkan API was built from the ground up with raw, uncompromising performance in mind. In traditional graphics APIs like OpenGL, the graphics driver spends a massive amount of processing power constantly double-checking your work, hunting for errors, and attempting to fix mistakes on the fly.

Vulkan achieves its incredible speed by completely removing this safety net.

Vulkan is known as a "thin API" or a "thin driver"—meaning it acts as a minimal, direct pass-through to the physical graphics hardware. Because the driver operates under the strict assumption that your application is perfectly written and will always abide by the official Vulkan specifications, it performs zero runtime error checking.

### The Paradox of Absolute Control

While this "thin" architecture makes Vulkan exceptionally fast and portable across diverse devices—ranging from mobile phones to high-end desktop rigs—it makes writing applications significantly more challenging. If your code makes a mistake (like requesting a resource that doesn't exist or mismanaging memory), the driver will not warn you. Instead, it will blindly execute the broken command, leading to unpredictable bugs, visual artifacts, or a hard crash of the entire graphics card.

### The Modular Solution: A Layered Architecture

To solve this problem without sacrificing performance, Vulkan was designed as a modular, layered API.

The absolute lowest layer is the Core Vulkan API, which talks directly to the hardware driver. However, Vulkan allows developers to intercept the communication between their application and the core API by inserting optional, plug-and-play components called Validation Layers.

Validation layers act as an invisible, intelligent intermediary. During development, you can turn these layers on. They catch every single command your application sends to the GPU, verify it against the thousands of pages of the official Vulkan rules, and output readable, real-time feedback.

When you are ready to ship your game or app to the public, you simply disable the validation layers. This leaves behind a perfectly clean, blazing-fast executable with absolutely zero debugging overhead.

### What Do Validation Layers Check For?

The standard validation layers provided in the LunarG SDK monitor your application for a wide variety of critical issues, categorized into four main areas:

**Incorrect API Usage**: They act as a strict referee, detecting if you provided an invalid setting, passed an improper parameter, or triggered functions in an illegal order.

**Resource Management & Memory Leaks**: They keep a close eye on your system's memory, ensuring that your application allocates, utilizes, and destroys GPU resources (like buffers, textures, and memory blocks) safely without leaking data.

**Synchronization Hazards**: Because Vulkan relies heavily on multi-threading, the CPU and GPU must be perfectly synchronized. Validation layers track your execution signals (fences, semaphores, and barriers) to flag dangerous situations where the CPU tries to read data at the exact same moment the GPU is writing to it.

**Performance Warnings**: Beyond tracking fatal errors, validation layers look for suboptimal behavior. If you are doing something that technically works but is highly inefficient for modern graphics hardware, the layers will give you a helpful performance warning so you can optimize your app.

![](./static/vulkan_app.png)

## The Vulkan Ecosystem & Helper Tools

Because Vulkan gives developers total control, it doesn't hold your hand. Doing everything from scratch requires thousands of lines of setup. To make life easier, the industry relies on standard tools and libraries:

- **LunarG**: While Khronos defines the rules of Vulkan, a company named LunarG (funded by companies like Valve and Google) builds the official Vulkan SDK (Software Development Kit). They provide the essential tools developers use to build Vulkan apps.

- **Vulkan Loader**: The system-level boundary that detects your specific graphics card driver (NVIDIA, AMD, Intel) and routes your code's commands to it.

- **Volk**: A helper tool that dynamically loads Vulkan features, boosting performance by cutting down entry-point overhead.

- **Vulkan Memory Allocator (VMA)**: In Vulkan, you have to manually manage GPU memory chunks. AMD created VMA to handle this complex task safely and efficiently behind the scenes.

- **Vulkan-Hpp**: A set of language bindings that allows developers writing in C++ to use clean, modern C++ syntax instead of traditional C styling.

## References

- [alain.xyz - Raw Vulkan](https://alain.xyz/blog/raw-vulkan#vertex-buffers): An overview on how to program a Hello Triangle Vulkan application from the ground up.