// C++ standard library
#include "vulkan/vulkan.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <format>
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
	vk::Extent2D           extent;
	vk::SurfaceFormatKHR   format;
	std::vector<vk::Image> images;
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
		ShowWindow();
		MainLoop();
		Cleanup();
	}

  private:
	vk::raii::Context                m_context;
	vk::raii::Instance               m_instance       = nullptr;
	vk::raii::PhysicalDevice         m_physicalDevice = nullptr;
	vk::raii::Queue                  m_graphicsQueue  = nullptr;
	vk::raii::Device                 m_device         = nullptr;
	vk::raii::SurfaceKHR             m_surface        = nullptr;
	vk::raii::SwapchainKHR           m_swapchain      = nullptr;
	SwapChainData                    m_swapchainData;
	vk::raii::DebugUtilsMessengerEXT m_debugMessenger = nullptr;

	SDL_Window* m_window       = nullptr;
	int         m_windowWidth  = 1280;
	int         m_windowHeight = 720;

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

		this->m_debugMessenger = this->m_instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
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
		std::vector<vk::LayerProperties> availableLayers = this->m_context.enumerateInstanceLayerProperties();

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
			this->m_instance = vk::raii::Instance(m_context, createInfo);
		} catch (const std::exception& err) {
			CleanupAndExit(std::format("[vulkan] Error: Instance creation failed: {}", err.what()));
		}
	}

	void CreateWindow() {
		spdlog::info("[Window] Initializing...");

		// Create SDL window
		SDL_WindowFlags windowFlags = (SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
		m_window                    = SDL_CreateWindow("Arctic Engine", this->m_windowWidth, this->m_windowHeight, windowFlags);

		if (!m_window)
			CleanupAndExit(std::format("[Window] Failed to initialize: {0}", SDL_GetError()));
	}

	void VulkanCreateSurface() {
		spdlog::info("[Vulkan] Creating surface...");
		VkSurfaceKHR rawSurface = nullptr;
		SDL_Vulkan_CreateSurface(m_window, *this->m_instance, nullptr, &rawSurface);
		this->m_surface = vk::raii::SurfaceKHR(this->m_instance, rawSurface);
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
				this->m_physicalDevice = physicalDevice;
				break;
			}
		}
		if (this->m_physicalDevice == nullptr)
			CleanupAndExit("[Vulkan] Found GPU's but none that are suitable!");
	}

	void VulkanCreateLogicalDevice() {
		spdlog::info("[Vulkan] Creating logical device...");
		// Get queue family that support the graphics and presentation
		std::vector<vk::QueueFamilyProperties> qfProperties    = this->m_physicalDevice.getQueueFamilyProperties();
		uint32_t                               selectedQfIndex = UINT32_MAX;
		for (uint32_t i = 0; i < qfProperties.size(); ++i) {
			auto qfp = qfProperties[i];
			if ((qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0) &&
				this->m_physicalDevice.getSurfaceSupportKHR(i, this->m_surface)) {
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
			featureChain = {{}, {.shaderDrawParameters = true}, {.dynamicRendering = true}, {.extendedDynamicState = true}};

		// Get required device extensions
		std::vector<const char*> requiredDeviceExtensions = {vk::KHRSwapchainExtensionName};

		// Create device
		vk::DeviceCreateInfo deviceCreateInfo{.pNext                   = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
											  .queueCreateInfoCount    = 1,
											  .pQueueCreateInfos       = &deviceQueueCreateInfo,
											  .enabledExtensionCount   = static_cast<uint32_t>(requiredDeviceExtensions.size()),
											  .ppEnabledExtensionNames = requiredDeviceExtensions.data()};
		this->m_device = vk::raii::Device(this->m_physicalDevice, deviceCreateInfo);

		// Get handle to graphics queue
		this->m_graphicsQueue = vk::raii::Queue(this->m_device, selectedQfIndex, 0);
	}

	void VulkanCreateSwapChain() {
		// Set format
		auto formats = this->m_physicalDevice.getSurfaceFormatsKHR(this->m_surface);
		if (formats.empty())
			CleanupAndExit("[Vulkan] Swapchain: No formats found!");
		const auto           kFormatIt      = std::ranges::find_if(formats, [](const auto& sf) {
			return sf.format == vk::Format::eB8G8R8A8Srgb && sf.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
		});
		vk::SurfaceFormatKHR selectedFormat = kFormatIt != formats.end() ? *kFormatIt : formats[0];

		// Set present mode
		auto presentModes = this->m_physicalDevice.getSurfacePresentModesKHR(this->m_surface);
		auto selectedPresentMode =
			std::ranges::any_of(presentModes, [](auto presentMode) { return presentMode == vk::PresentModeKHR::eMailbox; })
				? vk::PresentModeKHR::eMailbox
				: vk::PresentModeKHR::eFifo;

		// Set extent
		auto surfaceCaps = this->m_physicalDevice.getSurfaceCapabilitiesKHR(this->m_surface);
		auto extent =
			vk::Extent2D(std::clamp<uint32_t>(this->m_windowWidth, surfaceCaps.minImageExtent.width, surfaceCaps.maxImageExtent.width),
						 std::clamp<uint32_t>(this->m_windowHeight, surfaceCaps.minImageExtent.height, surfaceCaps.maxImageExtent.height));

		// Set image count
		uint32_t minImageCount = std::max(surfaceCaps.minImageCount, 3u);
		if ((surfaceCaps.maxImageCount > 0) && (surfaceCaps.maxImageCount < minImageCount))
			minImageCount = surfaceCaps.maxImageCount;

		// Create swapchain
		vk::SwapchainCreateInfoKHR swapchainCreateInfo{.surface          = this->m_surface,
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

		this->m_swapchain     = vk::raii::SwapchainKHR(this->m_device, swapchainCreateInfo);
		this->m_swapchainData = {.extent = extent, .format = selectedFormat};
	}

	void ShowWindow() {
		spdlog::info("[Window] Finalizing window setup...");
		SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		SDL_ShowWindow(m_window);
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

			// logic: todo
		}
	}

	void Cleanup() {
		spdlog::info("Cleaning up app...");
		// Vulkan
		this->m_swapchain.clear();
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