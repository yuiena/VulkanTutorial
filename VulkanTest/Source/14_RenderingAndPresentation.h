#pragma once
#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) 
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) 
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else 
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class HelloTriangleApplication {
public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	GLFWwindow* window;

	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR surface;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VkSwapchainKHR swapChain;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;

	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
	VkFence inFlightFence;

	void initWindow() 
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	}

	void initVulkan() {
		createInstance();
		setupDebugMessenger();
		/*
		Vulkan�� �÷����� ������ API������ Vulkan ��ü������ ������ �ý��۰� �������̽� �� ����� ����.
		��� Vulkan�� ������ �ý��۰��� ������ �����ؾ� �մϴ�.
		*/
		createSurface();
		// Physical Device(GPU) ����
		pickPhysicalDevice();
		// Logical Device : Physical Device�� ����ϱ� ���ؼ� ��� ��
		createLogicalDevice();
		/*
		Vulkan���� 'default framebuffer'��� ������ ��� �츮�� Screen�� �ð�ȭ ���ֱ� ���ؼ��� 
		�������� ���۸� �����ϱ� ���� ���۾��� �������. 
		�� �κ��� �ٷ� swap cahin�̸� vulkan ���� ��������� �����������.

		swap chain : screen�� ��µǱ� ���� ��ٸ��� image queue
		*/
		createSwapChain();
		/*
		������ swap chain�� �����ߴ�. ���� �̰��� ����Ϸ��� �� �ȿ� 
		VkImage�� ���� VkImageView(image�� �׼��� �ϴ� ���, �׼����� �̹��� �κ��� ���)�� �����ؾ� �Ѵ�.
		*/
		createImageViews();		
		//pipeline ���� ���� Vulkan���� ������ ���� ����� framebuffer attachment�� ���� �˷���� �Ѵ�.
		// color/depth buffer�� �մ��� � �������� ������ �۾��� ó���ؾ��ϴ� �� ���� render pass�� �����Ѵ�.
		createRenderPass();
		// ������ ������ ����ü, ������Ʈ�� �����Ͽ� graphic pipeline ����!
		createGraphicsPipeline();
		/*
		attachment�� VkFramebuffer�� ���εǼ� ���ε� ��� / framebuffer�� ��� VkImageView�� ����.
		�츮�� attachment�� �ϳ����� image�� swap chain�� ��ȯ�ϴ� image�� �����մϴ�.
		�� ���� swap chain�� �ִ� ��� image�� ���� fraem buffer�� �� �������ϰ� 
		drawing time�� ȹ���� �̹����� �����ϴ� frame buffer�� ����ؾ� �Ѵٴ� ���̴�.
		*/
		createFramebuffers();
		// Vulkan���� �����ϰ��� �ϴ� ��� operation���� command buffer�� ����ؾ� ��.
		// ��� command ����� ���� pool���� ����
		createCommandPool();
		// ���� command buffer�� �Ҵ��ϰ� �ű⿡ drawing command�� ����� �� �ִ�.
		createCommandBuffer();
		// image�� ȹ��Ǿ��� �������� �غ� �Ǿ��ٴ� signal�� ���� semaphore �׸���
		// �������� �Ϸ�ǰ� presentation�� �߻��� �� �ִٴ� signal�� ���� semaphore�� �ʿ��մϴ�.
		createSyncObjects();
	}

	void mainLoop()
	 {
		while (!glfwWindowShouldClose(window)) 
		{
			glfwPollEvents();
			drawFrame();
		}

		vkDeviceWaitIdle(device);
	}

	void cleanup()
	{
		vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
		vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
		vkDestroyFence(device, inFlightFence, nullptr);

		vkDestroyCommandPool(device, commandPool, nullptr);

		//frame buffer�� image view��� render pass ���Ŀ� ���� �Ǿ� ��.
		for (auto framebuffer : swapChainFramebuffers) 
		{
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}

		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);

		for (auto imageView : swapChainImageViews) 
		{
			vkDestroyImageView(device, imageView, nullptr);
		}

		vkDestroySwapchainKHR(device, swapChain, nullptr);
		vkDestroyDevice(device, nullptr);

		if (enableValidationLayers) 
		{
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}

		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);

		glfwDestroyWindow(window);

		glfwTerminate();
	}

	void createInstance() 
	{
		if (enableValidationLayers && !checkValidationLayerSupport()) 
		{
			throw std::runtime_error("validation layers requested, but not available!");
		}

		// application initialize data
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		// application info setting for instance create
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		// setting global extension
		auto extensions = getRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		// setting validation layer
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else {
			createInfo.enabledLayerCount = 0;

			createInfo.pNext = nullptr;
		}

		// finally! create vulkan Instance!
		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
	}

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) 
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		// �̺�Ʈ �ɰ��� ����
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		// �ݹ��� ȣ��ǵ��� ���ϴ� �̺�Ʈ ���� ����
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		//�ݹ� �Լ�
		createInfo.pfnUserCallback = debugCallback;
	}

	void setupDebugMessenger() 
	{
		if (!enableValidationLayers) return;

		//���� ������ ����� �޽����� �Ű������� �����ϴ� ����
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}

	
	void createSurface() 
	{
		// surface ? �������� �̹����� ǥ���� ��

		//�� �÷����� �ٸ� ������ ���� Vulkan surface ����
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to create window surface!");
		}
	}

	void pickPhysicalDevice() 
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0) 
		{
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		for (const auto& device : devices) 
		{
			if (isDeviceSuitable(device)) 
			{
				physicalDevice = device;
				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE) 
		{
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}

	void createLogicalDevice() 
	{
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		float queuePriority = 1.0f; // 0.0 ~ 1.0 ���̿��� ��� ����. �켱 ���� �Ҵ�.
		for (uint32_t queueFamily : uniqueQueueFamilies) 
		{
			//Queue Family���� �츮�� ���ϴ� queue�� ������ ���.
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures{};

		// VkDeviceQueueCreateInfo, VkPhysicalDeviceFeatures�� ���� VkDeviceCreateInfo ���� ����.
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		if (enableValidationLayers) 
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else 
		{
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device!");
		}

		// �� queue family���� queue �ڵ��� ã�ƿ´�. 
		// ( logical device, queue family, queue index, queue handle ������ ������)
		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
	}

	void createSwapChain() 
	{
		/*
		Swap Chain�� ������ surface�� ȣȯ���� ���� �� �������� ���� ���� �׸� ���� ���ǰ� �ʿ��մϴ�.

			1. Basic Surface capabilites(swap chain�� �ִ�/�ּ� �̹��� ����, �̹��� w/h �ִ� �ּ� ��)
			2. surface format(pixel format, color space)
			3. ��� ������ presentation ���
		*/
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

		/*
		swap chain�� ������ �ȴٴ� Ȯ���� ����ϴ�.
		���� ����ȭ�� Ȯ���ϱ� ���� 3���� Ÿ���� �����Ѵ�.
			
			1. surface format(color depth)
			2. presentation mode(�̹����� ȭ�鿡 �������ϱ� ���� ����)
			3. swap extent(swap chain�� �̹��� �ػ�)
		*/

		// 1. surface format(color depth)
		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		// 2. presentation mode(�̹����� ȭ�鿡 �������ϱ� ���� ����)
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		// 3. swap extent(swap chain�� �̹��� �ػ�)
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);


		// swap chain�� ����� �̹��� ������ ���Ѵ�. + 1 �� �ϴ� �ּ� 1�� �̻��� �̹����� ��û�ϰڴٴ� �ǹ��̴�.
		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) 
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		//���� Vulkan ������Ʈ ���ʴ�� swap chain�� ����ü�� ä������.
		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		// �̹����� �����ϴ� layer�� �� (3D APP�� �������� �ʴ� �̻� �׻� 1��.)
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		// Graphic queue family�� Presentation queue family�� �ٸ� ��� graphic queue���� 
		// swap chain �̹���, presentation queue�� �� �̹����� �����ϰ� ��.

		// Queue family���� ���Ǵ� S.C �̹��� �ڵ鸵 ����� ������.
		if (indices.graphicsFamily != indices.presentFamily) 
		{
			// ������� ������ ���� ���� �̹����� ���� Queue Family���� ��� ����.
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else 
		{
			// �̹����� �� Queue family���� �����ϰ� �ٸ� Q.F���� ����Ϸ��� ��� ��������� ������ ����.
			// �� �ɼ��� �ֻ��� ������ ���� ��.
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; 
		}

		// swap chain�� trasnform( ex: 90�� ȸ��.. ���� �ø� ��). �״�� �ѰŸ� current �ϸ� ��.
		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		//������ �ý��ۿ��� �ٸ� ������� ������ ���� ä�� ����� �ǰ��� ����.
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;// ���� ä�� ����
		createInfo.presentMode = presentMode;
		// ������ �ȼ��� �Ű澲�� �ʰڴٴ� ��
		createInfo.clipped = VK_TRUE;

		// ������ ������¡�� �� �� ���� swap chain�� �����ϱ� ���� �� �ʵ带 �����ؾ� ��.
		// ���������� �̰��� �ϴ� null�� ��.(���� �� �׻� ���� ����)
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		// ��� ���������� swap chain create!
		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to create swap chain!");
		}

		// ���� imageCount�� ���� �̹��� ������ ������ ��
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		// �����̳� ũ�⸦ �����ϰ�
		swapChainImages.resize(imageCount);
		// ���������� �̸� �ٽ� ȣ���Ͽ� �ڵ��� ���´�. �̴� �󸶵��� �� ���� ���� swapChain�� ������ �� �ֱ� �����̴�.
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	void createImageViews() 
	{
		// swap chain ������ �°� imageViews�� ��������
		swapChainImageViews.resize(swapChainImages.size());


		for (size_t i = 0; i < swapChainImages.size(); i++) 
		{
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapChainImageFormat;
			// color channel�� ���� �� �ֵ��� ����. (�ܻ� �ؽ�ó�� ���ٸ� ��� channel�� red�� ������ ���� ����.)
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			// �̹����� �뵵, � �κ��� �׼��� �ؾ��ϴ��� ���
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			// �� ���� ������ image View create!
			if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) 
			{
				throw std::runtime_error("failed to create image views!");
			}
		}
	}

	void createRenderPass() 
	{
		//swap chain �̹����� �� �ϳ��� ��Ÿ���� color buffer attachment
		// color(����) buffer(����) attachment(÷�ι�)
		//https://www.notion.so/VkAttachmentDescription-Manual-Page-774a0dde223c41939b99b4b4f04349c9
		VkAttachmentDescription colorAttachment{};
		//color attachment�� format�� swap chain image���� format�� ����.
		colorAttachment.format = swapChainImageFormat;
		// multisampling ���� �۾� ����( ���� �� ������ 1)
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

		//attatchmnet�� �����Ͱ� ������ ��/�Ŀ� ������ �� ������ ����
		// LOAD : ���� attachment ���� / CLEAR : ����� / DONT_CARE : ���� ������ undefined
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		// STORE : �������� ������ ���� �� ���� �� ���� / DONT_CARE : ������ �� undefined
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		//stencil ���� ���� (stencil ���۷� fragment ���� ���. ���⼱ ���x)
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		//Vulkan�� texture�� frame buffer�� Ư�� pixel format�� VkImage ������Ʈ�� ǥ�� ��.
		
		// render pass�� �����ϱ� �� ������ image layout ���� ����
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;//���� �̹����� � layout�̾��� ������� ��.
		// render pass�� ������ �� �ڵ������� ��ȯ�� layout�� ����.
		// ������ �Ŀ� swap chain�� ���� image�� presentation �� ���̱� ������ �Ʒ��� ���� ����.
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		//sub pass�� �ϳ� �̻��� attachment�� ������.
		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// ���� render pass�� �������� sub pass�� �����Ǵµ� sub pass�� 
		// ���� pass�� frame buffer ���뿡 �����ϴ� �ļ� ������ �۾��Դϴ�. (ex) post-processing)

		// ������ �ﰢ�� �ϳ� ���Ŵϱ� ���� sub pass ����.
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		// attachment �迭�� sub pass�� ����Ͽ� VkRenderPassCreateInfo ����ü�� ä��� ���� ����!
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to create render pass!");
		}
	}

	void createGraphicsPipeline() 
	{
		auto vertShaderCode = readFile("F:\\yuiena\\VULKAN\\VulkanTest\\Res\\shader\\vert.spv");
		auto fragShaderCode = readFile("F:\\yuiena\\VULKAN\\VulkanTest\\Res\\shader\\frag.spv");

		// ���������ο� �ڵ带 �����ϱ� ���� VkShaderModule ������Ʈ�� ���� �ؾ���.
		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		// shader�� ���� ����ϱ� ���� VkPipelineShaderStageCreateInfo�� ���� pipeline state�� ���� ����.
		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		/* ��Input Assetmbler	- raw vertex ������ ����
		   Vertex Shader		- ��� vertex���� ����, model->screen �������� 
		   Tessellation			- mesh ����Ƽ�� �ø��� ���� gemetry ����ȭ.(����, ��� ���� �� �����غ��̵��� ���)
		   Geometry shader		- ��� primitive�� ���� ����ǰ� �̸� �����ų� ���� �� ���� �� ���� primitive�� ��� ����.
								(tessellation�� ���������� �� ����, ���� ��ī���� ������ �� ���Ƽ� ��� �� ��� x, metal�� �� �ܰ� ����.)
		   ��Resterization		- primitive�� frament�� ����. fragment�� pixel ��ҷ� frame buffer�� ä��. 
								(ȭ�� �� frament ���, depth testing���� ���� fragment�� ���)
		   Fragment Shader		- ��Ƴ��� fragment�κ��� � framebuffer�� �������� � color, depth�� ������� ����.
		   ��Color blending		- frame buffer�ȿ� ������ pixel�� ��Ī�Ǵ� ���� fragment�� ȥȩ�Ѵ�.

		   ���� ���� �ܰ谡 fixed-function : �Ķ���͸� ���� operation�� ������ �� �ְ� �������� �̸� ���� ��
		   �������� programmable.
		*/
		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		//vertex shader�� ���޵Ǵ� vertex �������� ������ ���.
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		// ������ ���� ���ݰ� per-vertex���� per-instance���� ���� ����.
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;//optional
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;//optional

		// vertex�κ��� � ������ geometry�� �׸� ���̳� primitive restart�� Ȱ��ȭ �Ǿ��°�?
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		// viewport : image���� frame buffer�μ��� ��ġ?ũ��? transformation�� ����.
		// scissor : ������ screen�� pixel�� �׸��� ����
		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		// vertex shader�� geometry�� �޾� fragment shader�� ��ĥ�� fragment�� ��ȯ.
		// ���⼭ depth testing, face culiing, scissor test ����.
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		// true��� near/far plane�� ��� fragment�� ���Ǵ� ��� clamp��.
		// �� ������ shadow map���� Ư���� ��Ȳ���� ����. (GPU feature Ȱ��ȭ �ʿ�)
		rasterizer.depthClampEnable = VK_FALSE;
		// ture�� geometry�� rasteraizer �ܰ� ���� x 
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		// fragment�� �����Ǵ� ��� ���� 
		// FILL : fragment�� ä�� / LINE : ������ ������ �׸� / POINT : vertex�� ������ �׸�.
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		// fragment ���� �β� 
		rasterizer.lineWidth = 1.0f;
		// face culling ����
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		// multi sampling ����. anti-aliasing�� �����ϱ� ���� ��� �� �ϳ�.		
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		// depth�� stencil buffer�� ����ϱ� ���� ����.
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

		// color blending 
		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		// pipeline ����� ���� ������ �� �ִ� �͵�( viewport, scissor, line widht, blend constant�� )�� �ִµ� ���Ѵٸ� �׵��� ä���־����.
		std::vector<VkDynamicState> dynamicStates = 
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();


		//shader���� ���Ǵ� uniform���� global������ dynamic state �� �����ϰ� shader ����� ���� drawing �������� �ٲ� �� �ִ�.
		// �� uniform�� VkPipelineLayout ������Ʈ ������ ���� pipeline�� �����ϴ� ���� �����ȴ�.
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pushConstantRangeCount = 0;

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to create pipeline layout!");
		}

		/*
		������ ���� ��� ����ü�� ������Ʈ�� �����Ͽ� ���� graphics pipeline ���� ����!
			- shader stages :shader module ����
			- Fixed-function state : pipe line�� fixed-funtion�� ����
			- pieline layout : shader�� ���� �����Ǵ� uniform�� push ������ draw time�� ���� ����
			- render pass :pipeline �ܰ迡 �����ϴ� attachment�� �� ����
			
		�� ��� �͵��� ���յǾ�  graphics pipeline�� ����� ������ �����մϴ�.
		*/
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		// graphics pipeline create!
		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to create graphics pipeline!");
		}

		// pipeline ���� �� ������ ��.
		vkDestroyShaderModule(device, fragShaderModule, nullptr);
		vkDestroyShaderModule(device, vertShaderModule, nullptr);
	}

	void createFramebuffers() 
	{
		// swap chain�� ��� image�� ���� frame buffer���� ������ ��� ����
		swapChainFramebuffers.resize(swapChainImageViews.size());

		// imageView ������ŭ framebuffer ����
		for (size_t i = 0; i < swapChainImageViews.size(); i++) 
		{
			VkImageView attachments[] = 
			{
				swapChainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			//frame buffer�� ȣȯ�Ǵ� render pass ���(������ ������ Ÿ���� attachment�� ����ؾ� �Ѵٴ� �ǹ�)
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) 
			{
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
	}

	void createCommandPool() 
	{
		// command buffer�� queue�� �ϳ��� ���������ν� ���� ��. ��� queue�� ���� ��.
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		/*
		VK_COMMAND_POOL_CREATE_TRANSIENT_BIT : ���ο� command�� �ſ� ���� ���.
		VK_COMMNAD_POOL_CREATE_RESET_COMMAND_BUFFER_BIT : command buffer�� ���������� ���� �� �� ����.
		*/
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		// ���� ������ queue�� ����Ǿ� ��� ����.
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		//command pool ����!
		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to create command pool!");
		}
	}

	void createCommandBuffer() 
	{
		// command buffer�� vkAllocateCommandBuffers�� �Ҵ� ��.
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		// VK_COMMAND_BUFFER_LEVEL_PRIMARY : ������ ���� queue�� ����� �� ������ �ٸ� command buffer���� ȣ�� x
		// VK_COMMAND_BUFFER_LEVEL_SECONDARY : ���� ���� x, primary command buffer���� ȣ�� o
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}
	}

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) 
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		// command buffer ��� ����.
		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		//render pass �� �����ؼ� drawing�� �����ϰڴٴ� �ǹ�.
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		//render pass �ڽ�
		renderPassInfo.renderPass = renderPass;
		// ���ε� �� attachment
		renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
		// ������ ���� ũ�� ����
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChainExtent;
		//clear color ��
		VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		// ���� render pass�� ���� �Ǿ����ϴ�. command���� ����ϴ� ��� �Լ��� �ٵ��� ������ �ִ�
		// vkCmd ���λ�� �˾ƺ� �� �ֽ��ϴ�.
		// ù �Ķ���� : �׻� command�� ��� �� command buffer
		// �ι�° �Ķ���� : render pass ���� �׸�
		// ������ �Ķ���� : render pass������ drawing command�� ���� �����Ǵ��� ����.
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		//graphics pipeline ���ε�
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		vkCmdDraw(commandBuffer, 3, 1, 0, 0);

		vkCmdEndRenderPass(commandBuffer);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}

	void createSyncObjects() 
	{
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS) {
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}

	}

	void drawFrame() 
	{
		/*
			draw�ϱ� ���� �ؾ��ϴ� ��.

			1. swap chain���� ���� image ȹ��
			2. frame buffer���� �ش� image�� attachment�� command buffer ����
			3. presentation�� ���� swap chain�� image ��ȯ.
		*/
		vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
		vkResetFences(device, 1, &inFlightFence);

		//1. swap chain���� ���� image ȹ��
		uint32_t imageIndex;
		vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

		vkResetCommandBuffer(commandBuffer, /*VkCommandBufferResetFlagBits*/ 0);
		recordCommandBuffer(commandBuffer, imageIndex);

		// queue submit(����) �� ����ȭ
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;


		//semaphore���� ������ ���۵Ǳ� ���� ��ٷ��� �ϴ���, pipeline�� stage(��)�� ��ٷ����ϴ��� ����.
		VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		// 2. frame buffer���� �ش� image�� attachment�� command buffer ����
		// swap chain image�� color attachment�� ���ε��ϴ� command buffer ����.
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		//������ �Ϸ� ������ signal���� semaphore.
		VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		//������ �����ߴ� �͵�� graphics queue�� command buffer ���� ����.
		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		// 3. presentation�� ���� swap chain�� image ��ȯ.
		// frame�� drawing�ϴ� ������ �ܰ�.
		// ����� swap chain���� �ٽ� �����Ͽ� ���������� ȭ�鿡 ǥ���ϴ� ���̴�.
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		// presentatin�� �߻��ϱ� ������ ��ٸ� semaphore ����
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		//image�� ǥ���� swap chain��� �� swap chain�� index
		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1; //�׻� 1
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;

		// swap chain���� image�� ǥ���϶�� ��û ����!!
		vkQueuePresentKHR(presentQueue, &presentInfo);
	}

	VkShaderModule createShaderModule(const std::vector<char>& code) 
	{
		//  VkShaderModule ������Ʈ�� shader ����.
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to create shader module!");
		}

		return shaderModule;
	}

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) 
	{
		for (const auto& availableFormat : availableFormats) 
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) 
	{
		for (const auto& availablePresentMode : availablePresentModes) 
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) 
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) 
	{
		SwapChainSupportDetails details;
			
		//1. Basic Surface capabilites(swap chain�� �ִ� / �ּ� �̹��� ����, �̹��� w / h �ִ� �ּ� ��)
		// ������ Physical Device(GPU)�� ������ Surface�� ����Ͽ� �����Ǵ� capability�� ������.
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		//2. surface format(pixel format, color space)
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

		if (formatCount != 0) 
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		//3. ��� ������ presentation ���
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

		if (presentModeCount != 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	bool isDeviceSuitable(VkPhysicalDevice device) 
	{
		QueueFamilyIndices indices = findQueueFamilies(device);

		bool extensionsSupported = checkDeviceExtensionSupport(device);

		bool swapChainAdequate = false;
		if (extensionsSupported) 
		{
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		return indices.isComplete() && extensionsSupported && swapChainAdequate;
	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice device) 
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) 
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		// queue family ����Ʈ�� ����
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies) 
		{
			//VK_QUEUE_GRAPHICS_BIT �� �����ϴ� �ּ� �ϳ��� queue family�� ã�ƾ� ��.
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
			{
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

			if (presentSupport) 
			{
				indices.presentFamily = i;
			}

			if (indices.isComplete()) 
			{
				break;
			}

			i++;
		}

		return indices;
	}

	std::vector<const char*> getRequiredExtensions() 
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (enableValidationLayers) 
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	bool checkValidationLayerSupport() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : validationLayers) {
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}

		return true;
	}

	static std::vector<char> readFile(const std::string& filename) 
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			throw std::runtime_error("failed to open file!");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}
};

int Run() 
{
	HelloTriangleApplication app;

	try 
	{
		app.run();
	}
	catch (const std::exception& e) 
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}