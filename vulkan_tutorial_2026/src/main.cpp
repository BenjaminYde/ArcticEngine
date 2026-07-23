// C++ standard library
#include "vulkan/vulkan.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <vector>

// Libraries
#include <spdlog/spdlog.h>

// Vulkan C
#include <vulkan/vk_platform.h>

// Vulkan CPP
#include <vulkan/vulkan_raii.hpp>

// SDL3
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

// Validation layers
const std::vector<char const*> kValidationLayers = {"VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
constexpr bool ENABLE_VALIDATION_LAYERS = false;
#else
constexpr bool kEnableValidationLayers = true;
#endif

namespace arctic {

struct SwapChainData {
	vk::Extent2D                     extent;
	vk::SurfaceFormatKHR             surfaceFormat;
	std::vector<vk::Image>           images;
	std::vector<vk::raii::ImageView> imageViews;
};

class Application {
  public:
	void Run() {
		VulkanCreateInstance();
		SetupDebugMessenger();
		VulkanSelectPhysicalDevice();
		CreateWindow();
		VulkanCreateSurface();
		VulkanCreateLogicalDevice();
		VulkanCreateSwapChain();
		VulkanCreateImageViews();
		VulkanCreateGraphicsPipeline();
		VulkanCreateCommandPool();
		VulkanCreateCommandBuffer();
		VulkanCreateSyncObjects();
		ShowWindow();
		MainLoop();
		Cleanup();
	}

  private:
	vk::raii::Context m_context;

	vk::raii::Instance       m_instance           = nullptr;
	vk::raii::PhysicalDevice m_physicalDevice     = nullptr;
	vk::raii::Queue          m_graphicsQueue      = nullptr;
	uint32_t                 m_graphicsQueueIndex = UINT_MAX;
	vk::raii::Device         m_device             = nullptr;

	vk::raii::SurfaceKHR   m_surface   = nullptr;
	vk::raii::SwapchainKHR m_swapchain = nullptr;
	SwapChainData          m_swapchainData;

	vk::raii::PipelineLayout m_pipelineLayout   = nullptr;
	vk::raii::Pipeline       m_graphicsPipeline = nullptr;

	vk::raii::CommandPool   m_commandPool   = nullptr;
	vk::raii::CommandBuffer m_commandBuffer = nullptr;

	vk::raii::Semaphore m_presentCompleteSemaphore = nullptr;
	vk::raii::Semaphore m_renderFinishedSemaphore  = nullptr;
	vk::raii::Fence     m_drawFence                = nullptr;

	vk::raii::DebugUtilsMessengerEXT m_debugMessenger = nullptr;

	SDL_Window* m_window       = nullptr;
	int         m_windowWidth  = 1280;
	int         m_windowHeight = 720;

	static const std::filesystem::path& GetEnvVarProjectRootFolder() {
		static const std::filesystem::path kValue{PROJECT_ROOT_FOLDER};
		return kValue;
	}

	static std::vector<char> ReadFile(const std::filesystem::path& file) {
		std::ifstream stream(file.string(), std::ios::ate | std::ios::binary);
		if (!stream.is_open())
			return {};

		std::vector<char> buffer(stream.tellg());
		stream.seekg(0, std::ifstream::beg);
		stream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
		stream.close();
		return buffer;
	}

	static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT      severity,
														  vk::DebugUtilsMessageTypeFlagsEXT             type,
														  const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
														  void* /*pUserData*/) {
		if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
			spdlog::error("[Vulkan] Validation layer: type {0} msg: {1}", to_string(type), pCallbackData->pMessage);
		else if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
			spdlog::warn("[Vulkan] Validation layer: type {0} msg: {1}", to_string(type), pCallbackData->pMessage);
		return vk::False;
	}

	void SetupDebugMessenger() {
		if (!kEnableValidationLayers)
			return;

		vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
															vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
															vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);

		vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
														   vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
														   vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

		vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{.messageSeverity = severityFlags,
																			  .messageType     = messageTypeFlags,
																			  .pfnUserCallback = &DebugCallback};

		m_debugMessenger = m_instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
	}

	void VulkanCreateInstance() {
		// Get required vulkan validation layers
		spdlog::info("[Vulkan] Querying Vulkan validation layers...");

		std::vector<const char*> requiredLayers;
		if (kEnableValidationLayers) {
			spdlog::info("[Vulkan] Enabling validation layers...");
			requiredLayers.assign(kValidationLayers.begin(), kValidationLayers.end());
		}

		// Check if the required validation layers are supported by the Vulkan implementation
		std::vector<vk::LayerProperties> availableLayers = m_context.enumerateInstanceLayerProperties();

		if (kEnableValidationLayers) {
			spdlog::info("  Available instance layers:");
			for (const auto& layer : availableLayers) {
				spdlog::info("    {}", layer.layerName.data());
			}
		}

		for (const char* requiredLayer : requiredLayers) {
			bool found = std::ranges::any_of(availableLayers, [&](const vk::LayerProperties& layer) {
				return std::strcmp(layer.layerName.data(), requiredLayer);
			});
			if (!found)
				CleanupAndExit(std::format("[vulkan] Error: Required layer not supported: {}", requiredLayer));
		}

		// Get required SDL Vulkan extensions
		spdlog::info("[Vulkan] Querying SDL Vulkan extensions...");
		std::vector<const char*> requiredExtensions;

		// Add SDL extensions to the list of required extensions
		SDL_Init(SDL_INIT_VIDEO);
		uint32_t           sdlExtensionCount   = 0;
		char const* const* sdlExtensionsCStyle = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);
		requiredExtensions.insert(requiredExtensions.end(), sdlExtensionsCStyle, sdlExtensionsCStyle + sdlExtensionCount);

		// Add other extensions
		requiredExtensions.emplace_back("VK_KHR_portability_enumeration"); // mandatory
		if (kEnableValidationLayers) {
			requiredExtensions.emplace_back(vk::EXTDebugUtilsExtensionName);
		}

		// Debug extension
		if (kEnableValidationLayers) {
			spdlog::info("  Required extensions:");
			for (const auto& extension : requiredExtensions) {
				spdlog::info("    {}", extension);
			}
		}

		// Check if the required extensions are supported by the Vulkan implementation
		std::vector<vk::ExtensionProperties> availableExtensions = m_context.enumerateInstanceExtensionProperties();
		if (kEnableValidationLayers) {
			spdlog::info("  Available extensions:");
			for (const auto& extension : availableExtensions) {
				spdlog::info("    {}", extension.extensionName.data());
			}
		}

		for (const auto* requiredExtension : requiredExtensions) {
			bool isSupported = std::ranges::any_of(availableExtensions, [requiredExtension](const vk::ExtensionProperties& prop) {
				return std::strcmp(prop.extensionName, requiredExtension);
			});
			if (!isSupported)
				CleanupAndExit(std::format("[vulkan] Error: Required layer not supported: {}", requiredExtension));
		}

		// Create instance
		spdlog::info("[Vulkan] Creating instance...");
		vk::ApplicationInfo appInfo{.sType              = vk::StructureType::eApplicationInfo,
									.pApplicationName   = "Arctic Game",
									.applicationVersion = vk::makeVersion(1, 0, 0),
									.pEngineName        = "Arctic Engine",
									.engineVersion      = vk::makeVersion(1, 0, 0),
									.apiVersion         = vk::ApiVersion14};

		vk::InstanceCreateInfo createInfo{.pApplicationInfo        = &appInfo,
										  .enabledLayerCount       = static_cast<uint32_t>(requiredLayers.size()),
										  .ppEnabledLayerNames     = requiredLayers.data(),
										  .enabledExtensionCount   = static_cast<uint32_t>(requiredExtensions.size()),
										  .ppEnabledExtensionNames = requiredExtensions.data()};

		try {
			m_instance = vk::raii::Instance(m_context, createInfo);
		} catch (const std::exception& err) {
			CleanupAndExit(std::format("[vulkan] Error: Instance creation failed: {}", err.what()));
		}
	}

	void CreateWindow() {
		spdlog::info("[Window] Initializing...");

		// Create SDL window
		SDL_WindowFlags windowFlags = (SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
		m_window                    = SDL_CreateWindow("Arctic Engine", m_windowWidth, m_windowHeight, windowFlags);

		if (!m_window)
			CleanupAndExit(std::format("[Window] Failed to initialize: {0}", SDL_GetError()));
	}

	void VulkanCreateSurface() {
		spdlog::info("[Vulkan] Creating surface...");
		VkSurfaceKHR rawSurface = nullptr;
		SDL_Vulkan_CreateSurface(m_window, *m_instance, nullptr, &rawSurface);
		m_surface = vk::raii::SurfaceKHR(m_instance, rawSurface);
	}

	bool IsDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice) {
		// Check properties
		auto deviceProperties = physicalDevice.getProperties();
		if (deviceProperties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu && deviceProperties.apiVersion < vk::ApiVersion13)
			return false;

		// Check features
		auto deviceFeatures = physicalDevice.getFeatures();
		if (!deviceFeatures.geometryShader)
			return false;

		// Check queue families
		auto queueFamilies = physicalDevice.getQueueFamilyProperties();
		bool supportGraphics =
			std::ranges::any_of(queueFamilies, [](auto const& qfp) { return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics); });
		if (!supportGraphics)
			return false;

		// Check device extensions
		std::vector<const char*> requiredDeviceExtensions  = {vk::KHRSwapchainExtensionName};
		auto                     availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();

		for (const auto* requiredDeviceExtension : requiredDeviceExtensions) {
			bool isSupported = std::ranges::any_of(availableDeviceExtensions, [requiredDeviceExtension](auto const& e) {
				return std::strcmp(e.extensionName, requiredDeviceExtension);
			});
			if (!isSupported)
				return false;
		}

		// Check features 2
		auto features =
			physicalDevice.template getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features,
												 vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
		bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
										features.template get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
										features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
										features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;
		return supportsRequiredFeatures;
	}

	void VulkanSelectPhysicalDevice() {
		spdlog::info("[Vulkan] Selecting physical device...");

		// Get physical devices
		std::vector<vk::raii::PhysicalDevice> physicalDevices = m_instance.enumeratePhysicalDevices();

		if (physicalDevices.empty())
			CleanupAndExit("[Vulkan] Failed to find GPU's with Vulkan support!");

		// Find suitable device
		for (const vk::raii::PhysicalDevice& physicalDevice : physicalDevices) {
			if (IsDeviceSuitable(physicalDevice)) {
				m_physicalDevice = physicalDevice;
				break;
			}
		}
		if (m_physicalDevice == nullptr)
			CleanupAndExit("[Vulkan] Found GPU's but none that are suitable!");
	}

	void VulkanCreateLogicalDevice() {
		spdlog::info("[Vulkan] Creating logical device...");
		// Get queue family that support the graphics and presentation
		std::vector<vk::QueueFamilyProperties> qfProperties    = m_physicalDevice.getQueueFamilyProperties();
		uint32_t                               selectedQfIndex = UINT32_MAX;
		for (uint32_t i = 0; i < qfProperties.size(); ++i) {
			auto qfp = qfProperties[i];
			if ((qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0) &&
				m_physicalDevice.getSurfaceSupportKHR(i, m_surface)) {
				selectedQfIndex = i;
				break;
			}
		}
		if (selectedQfIndex == UINT32_MAX)
			CleanupAndExit("[Vulkan] Logical device creation: Could not find a valid QF for graphics and presentation!");

		float                     queuePriority         = 0.5f;
		vk::DeviceQueueCreateInfo deviceQueueCreateInfo = {.queueFamilyIndex = selectedQfIndex,
														   .queueCount       = 1,
														   .pQueuePriorities = &queuePriority};

		// Create a chain of feature structures
		vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan13Features,
						   vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
			featureChain = {{},
							{.shaderDrawParameters = true},
							{.synchronization2 = true, .dynamicRendering = true},
							{.extendedDynamicState = true}};

		// Get required device extensions
		std::vector<const char*> requiredDeviceExtensions = {vk::KHRSwapchainExtensionName};

		// Create device
		vk::DeviceCreateInfo deviceCreateInfo{.pNext                   = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
											  .queueCreateInfoCount    = 1,
											  .pQueueCreateInfos       = &deviceQueueCreateInfo,
											  .enabledExtensionCount   = static_cast<uint32_t>(requiredDeviceExtensions.size()),
											  .ppEnabledExtensionNames = requiredDeviceExtensions.data()};
		m_device = vk::raii::Device(m_physicalDevice, deviceCreateInfo);

		// Get handle to graphics queue
		m_graphicsQueue      = vk::raii::Queue(m_device, selectedQfIndex, 0);
		m_graphicsQueueIndex = selectedQfIndex;
	}

	void VulkanCreateSwapChain() {
		// Set format
		auto formats = m_physicalDevice.getSurfaceFormatsKHR(m_surface);
		if (formats.empty())
			CleanupAndExit("[Vulkan] Swapchain: No formats found!");
		const auto           kFormatIt      = std::ranges::find_if(formats, [](const auto& sf) {
			return sf.format == vk::Format::eB8G8R8A8Srgb && sf.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
		});
		vk::SurfaceFormatKHR selectedFormat = kFormatIt != formats.end() ? *kFormatIt : formats[0];

		// Set present mode
		auto presentModes = m_physicalDevice.getSurfacePresentModesKHR(m_surface);
		auto selectedPresentMode =
			std::ranges::any_of(presentModes, [](auto presentMode) { return presentMode == vk::PresentModeKHR::eMailbox; })
				? vk::PresentModeKHR::eMailbox
				: vk::PresentModeKHR::eFifo;

		// Set extent
		auto surfaceCaps = m_physicalDevice.getSurfaceCapabilitiesKHR(m_surface);
		auto extent =
			vk::Extent2D(std::clamp<uint32_t>(m_windowWidth, surfaceCaps.minImageExtent.width, surfaceCaps.maxImageExtent.width),
						 std::clamp<uint32_t>(m_windowHeight, surfaceCaps.minImageExtent.height, surfaceCaps.maxImageExtent.height));

		// Set image count
		uint32_t minImageCount = std::max(surfaceCaps.minImageCount, 3u);
		if ((surfaceCaps.maxImageCount > 0) && (surfaceCaps.maxImageCount < minImageCount))
			minImageCount = surfaceCaps.maxImageCount;

		// Create swapchain
		vk::SwapchainCreateInfoKHR swapchainCreateInfo{.surface          = m_surface,
													   .minImageCount    = minImageCount,
													   .imageFormat      = selectedFormat.format,
													   .imageColorSpace  = selectedFormat.colorSpace,
													   .imageExtent      = extent,
													   .imageArrayLayers = 1,
													   .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,
													   .imageSharingMode = vk::SharingMode::eExclusive,
													   .preTransform     = surfaceCaps.currentTransform,
													   .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
													   .presentMode      = selectedPresentMode,
													   .clipped          = true,
													   .oldSwapchain     = nullptr};

		m_swapchain     = vk::raii::SwapchainKHR(m_device, swapchainCreateInfo);
		m_swapchainData = {.extent = extent, .surfaceFormat = selectedFormat, .images = m_swapchain.getImages()};
	}

	void VulkanCreateImageViews() {
		// Loop over existing swapchain images
		for (auto& image : m_swapchainData.images) {
			// Create image view
			vk::ImageViewCreateInfo imageViewCreateInfo{.image            = image,
														.viewType         = vk::ImageViewType::e2D,
														.format           = m_swapchainData.surfaceFormat.format,
														.components       = {.r = vk::ComponentSwizzle::eIdentity,
																			 .g = vk::ComponentSwizzle::eIdentity,
																			 .b = vk::ComponentSwizzle::eIdentity},
														.subresourceRange = {.aspectMask     = vk::ImageAspectFlagBits::eColor,
																			 .baseMipLevel   = 0,
																			 .levelCount     = 1,
																			 .baseArrayLayer = 0,
																			 .layerCount     = 1}

			};
			auto                    imageView = vk::raii::ImageView(m_device, imageViewCreateInfo);
			m_swapchainData.imageViews.emplace_back(std::move(imageView));
		}
	}

	vk::raii::ShaderModule CreateShadeModule(const std::vector<char>& code) const {
		// convert code to something required for the shader module
		std::vector<uint32_t> spirvCode(code.size() / sizeof(uint32_t));
		std::memcpy(spirvCode.data(), code.data(), code.size());

		// create shader module
		vk::ShaderModuleCreateInfo createInfo{.codeSize = spirvCode.size() * sizeof(uint32_t), .pCode = spirvCode.data()};
		vk::raii::ShaderModule     shaderModule{m_device, createInfo};
		return shaderModule;
	}

	void VulkanCreateGraphicsPipeline() {
		// read shader
		auto shaderFile = GetEnvVarProjectRootFolder() / "shaders/shader.spv";
		auto shaderCode = ReadFile(shaderFile);

		// create shader
		auto shaderModule = CreateShadeModule(shaderCode);

		// create pipeline
		vk::PipelineShaderStageCreateInfo vertShaderStageInfo{.stage  = vk::ShaderStageFlagBits::eVertex,
															  .module = shaderModule,
															  .pName  = "vertMain"};

		vk::PipelineShaderStageCreateInfo fragShaderStageInfo{.stage  = vk::ShaderStageFlagBits::eFragment,
															  .module = shaderModule,
															  .pName  = "fragMain"};

		std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages{vertShaderStageInfo, fragShaderStageInfo};

		// create dynamic state
		std::vector<vk::DynamicState>      dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
		vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo{.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
																  .pDynamicStates    = dynamicStates.data()};

		// create vertex input
		vk::PipelineVertexInputStateCreateInfo   vertexInputInfo;
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{.topology = vk::PrimitiveTopology::eTriangleList};

		// viewport
		vk::Viewport                        viewport{.x        = 0.0f,
													 .y        = 0.0f,
													 .width    = static_cast<float>(m_swapchainData.extent.width),
													 .height   = static_cast<float>(m_swapchainData.extent.height),
													 .minDepth = 0.0f,
													 .maxDepth = 1.0f};
		vk::Rect2D                          scissor{.offset = vk::Offset2D{.x = 0, .y = 0}, .extent = m_swapchainData.extent};
		vk::PipelineViewportStateCreateInfo viewportState{.viewportCount = 1,
														  .pViewports    = &viewport,
														  .scissorCount  = 1,
														  .pScissors     = &scissor};

		// rasterizer
		vk::PipelineRasterizationStateCreateInfo rasterizer{.depthClampEnable        = vk::False,
															.rasterizerDiscardEnable = vk::False,
															.polygonMode             = vk::PolygonMode::eFill,
															.cullMode                = vk::CullModeFlagBits::eBack,
															.frontFace               = vk::FrontFace::eClockwise,
															.depthBiasEnable         = vk::False,
															.lineWidth               = 1.0f};

		// multisampling
		vk::PipelineMultisampleStateCreateInfo multisampling{.rasterizationSamples = vk::SampleCountFlagBits::e1,
															 .sampleShadingEnable  = vk::False};

		// color blending
		vk::PipelineColorBlendAttachmentState colorBlendAttachment{.blendEnable = vk::False,
																   .colorWriteMask =
																	   vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
																	   vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
		vk::PipelineColorBlendStateCreateInfo colorBlending{.logicOpEnable   = vk::False,
															.logicOp         = vk::LogicOp::eCopy,
															.attachmentCount = 1,
															.pAttachments    = &colorBlendAttachment};

		// pipeline layout
		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{.setLayoutCount = 0, .pushConstantRangeCount = 0};
		m_pipelineLayout = vk::raii::PipelineLayout(m_device, pipelineLayoutInfo);

		// create pipeline
		vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain{
			vk::GraphicsPipelineCreateInfo{.stageCount          = 2,
										   .pStages             = shaderStages.data(),
										   .pVertexInputState   = &vertexInputInfo,
										   .pInputAssemblyState = &inputAssembly,
										   .pViewportState      = &viewportState,
										   .pRasterizationState = &rasterizer,
										   .pMultisampleState   = &multisampling,
										   .pColorBlendState    = &colorBlending,
										   .pDynamicState       = &dynamicStateCreateInfo,
										   .layout              = m_pipelineLayout,
										   .renderPass          = nullptr},
			vk::PipelineRenderingCreateInfo{.colorAttachmentCount = 1, .pColorAttachmentFormats = &m_swapchainData.surfaceFormat.format}};
		m_graphicsPipeline = vk::raii::Pipeline(m_device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
	}

	void VulkanCreateCommandPool() {
		vk::CommandPoolCreateInfo poolInfo{.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
										   .queueFamilyIndex = m_graphicsQueueIndex};
		m_commandPool = vk::raii::CommandPool(m_device, poolInfo);
	}

	void VulkanCreateCommandBuffer() {
		vk::CommandBufferAllocateInfo allocInfo{.commandPool        = m_commandPool,
												.level              = vk::CommandBufferLevel::ePrimary,
												.commandBufferCount = 1};
		m_commandBuffer = std::move(vk::raii::CommandBuffers(m_device, allocInfo).front());
	}


	/**
	 * @brief Transitions a swapchain image from one layout to another using Vulkan 1.3 Synchronization2.
	 *
	 * Inserts an execution and memory dependency barrier into the current command buffer
	 * to safely transition image layout and synchronize memory access across pipeline stages.
	 *
	 * @param imageIndex      Index of the target image in the swapchain image array.
	 * @param oldLayout       Current layout of the image before the barrier.
	 * @param newLayout       Target layout the image will transition into.
	 * @param srcAccessMask   Memory operations to flush/finish before the transition (e.g., eColorAttachmentWrite).
	 * @param dstAccessMask   Memory operations allowed after the transition (e.g., eMemoryRead).
	 * @param srcStageMask    Pipeline stages that must complete before the barrier (e.g., eColorAttachmentOutput).
	 * @param dstStageMask    Pipeline stages that must wait for the barrier before starting (e.g., eBottomOfPipe).
	 */
	void VulkanTansitionImageLayout(uint32_t imageIndex, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
									vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask, vk::PipelineStageFlags2 srcStageMask,
									vk::PipelineStageFlags2 dstStageMask) {
		vk::ImageMemoryBarrier2 barrier = {.srcStageMask        = srcStageMask,
										   .srcAccessMask       = srcAccessMask,
										   .dstStageMask        = dstStageMask,
										   .dstAccessMask       = dstAccessMask,
										   .oldLayout           = oldLayout,
										   .newLayout           = newLayout,
										   .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
										   .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
										   .image               = m_swapchainData.images[imageIndex],
										   .subresourceRange    = {.aspectMask     = vk::ImageAspectFlagBits::eColor,
																   .baseMipLevel   = 0,
																   .levelCount     = 1,
																   .baseArrayLayer = 0,
																   .layerCount     = 1}};

		vk::DependencyInfo dependencyInfo = {.dependencyFlags = {}, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier};
		m_commandBuffer.pipelineBarrier2(dependencyInfo);
	}

	void VulkanRecordCommandBuffer(uint32_t imageIndex) {

		// start command
		m_commandBuffer.begin({});

		// prepare image layout
		VulkanTansitionImageLayout(imageIndex, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
								   {},                                         // srcAccessMask (no need to wait for previous operations)
								   vk::AccessFlagBits2::eColorAttachmentWrite, // dstAccessMask
								   vk::PipelineStageFlagBits2::eColorAttachmentOutput, // srcStage
								   vk::PipelineStageFlagBits2::eColorAttachmentOutput  // dstStage
		);

		// begin render
		vk::ClearValue              clearColor     = {.color = {.float32 = std::array{0.0f, 0.0f, 0.0f, 1.0f}}};
		vk::RenderingAttachmentInfo attachmentInfo = {.imageView   = m_swapchainData.imageViews[imageIndex],
													  .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
													  .loadOp      = vk::AttachmentLoadOp::eClear,
													  .storeOp     = vk::AttachmentStoreOp::eStore,
													  .clearValue  = clearColor};
		vk::RenderingInfo           renderingInfo  = {.renderArea           = {.offset = {.x = 0, .y = 0}, .extent = m_swapchainData.extent},
													  .layerCount           = 1,
													  .colorAttachmentCount = 1,
													  .pColorAttachments    = &attachmentInfo};
		m_commandBuffer.beginRendering(renderingInfo);

		// render
		m_commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_graphicsPipeline);
		m_commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(m_swapchainData.extent.width),
													static_cast<float>(m_swapchainData.extent.height), 0.0f, 1.0f));
		m_commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), m_swapchainData.extent));
		m_commandBuffer.draw(3, 1, 0, 0);

		// stop render
		m_commandBuffer.endRendering();

		// After rendering, transition the swapchain image to vk::ImageLayout::ePresentSrcKHR
		VulkanTansitionImageLayout(imageIndex, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
								   vk::AccessFlagBits2::eColorAttachmentWrite,         // srcAccessMask
								   {},                                                 // dstAccessMask
								   vk::PipelineStageFlagBits2::eColorAttachmentOutput, // srcStage
								   vk::PipelineStageFlagBits2::eBottomOfPipe           // dstStage
		);

		// end command
		m_commandBuffer.end();
	}

	void VulkanCreateSyncObjects() {
		m_presentCompleteSemaphore = vk::raii::Semaphore(m_device, vk::SemaphoreCreateInfo());
		m_renderFinishedSemaphore  = vk::raii::Semaphore(m_device, vk::SemaphoreCreateInfo());
		m_drawFence                = vk::raii::Fence(m_device, {.flags = vk::FenceCreateFlagBits::eSignaled});
	}

	void ShowWindow() {
		spdlog::info("[Window] Finalizing window setup...");
		SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		SDL_ShowWindow(m_window);
	}

	void VulkanDrawFrame() {
		// wait until previous frame is drawn
		auto fenceResult = m_device.waitForFences(*m_drawFence, vk::True, UINT64_MAX);
		if (fenceResult != vk::Result::eSuccess) {
			CleanupAndExit("Failed to wait for fence!");
		}
		m_device.resetFences(*m_drawFence);

		// acquire swapchain image
		auto [result, imageIndex] = m_swapchain.acquireNextImage(UINT64_MAX, *m_presentCompleteSemaphore, nullptr);

		// record command buffer
		VulkanRecordCommandBuffer(imageIndex);

		// submit command buffer
		vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
		const vk::SubmitInfo   kSubmitInfo{.waitSemaphoreCount   = 1,
										   .pWaitSemaphores      = &*m_presentCompleteSemaphore,
										   .pWaitDstStageMask    = &waitDestinationStageMask,
										   .commandBufferCount   = 1,
										   .pCommandBuffers      = &*m_commandBuffer,
										   .signalSemaphoreCount = 1,
										   .pSignalSemaphores    = &*m_renderFinishedSemaphore};

		m_graphicsQueue.submit(kSubmitInfo, *m_drawFence);

		// present render
		const vk::PresentInfoKHR kPresentInfoKhr{.waitSemaphoreCount = 1,
												 .pWaitSemaphores    = &*m_renderFinishedSemaphore,
												 .swapchainCount     = 1,
												 .pSwapchains        = &*m_swapchain,
												 .pImageIndices      = &imageIndex};
		result = m_graphicsQueue.presentKHR(kPresentInfoKhr);
	}

	void MainLoop() {
		SDL_Event event;
		bool      running = true;
		while (running) {
			// check input
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_EVENT_QUIT)
					running = false;
			}
			VulkanDrawFrame();
		}
		m_device.waitIdle();
	}

	void Cleanup() {
		spdlog::info("Cleaning up app...");
		// Vulkan
		m_swapchain.clear();
		// SDL
		SDL_DestroyWindow(m_window);
		SDL_Quit();
		spdlog::info("Done!");
	}

	void CleanupAndExit(const std::string& msg) {
		spdlog::error(msg);
		Cleanup();
		std::exit(EXIT_FAILURE);
	}
};
} // namespace arctic

int main() {
	auto app = arctic::Application();
	app.Run();
	return EXIT_SUCCESS;
}