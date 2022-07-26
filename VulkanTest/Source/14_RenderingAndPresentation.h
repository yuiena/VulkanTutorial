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
		Vulkan은 플랫폼과 무관한 API임으로 Vulkan 자체에서는 윈도우 시스템과 인터페이스 할 기능이 없다.
		고로 Vulkan과 윈도우 시스템간의 연결을 설정해야 합니다.
		*/
		createSurface();
		// Physical Device(GPU) 선택
		pickPhysicalDevice();
		// Logical Device : Physical Device와 통신하기 위해서 사용 됨
		createLogicalDevice();
		/*
		Vulkan에는 'default framebuffer'라는 개념이 없어서 우리가 Screen에 시각화 해주기 위해서는 
		렌더링할 버퍼를 소유하기 위한 밑작업을 해줘야함. 
		이 부분이 바로 swap cahin이며 vulkan 에서 명시적으로 생성해줘야함.

		swap chain : screen에 출력되기 전에 기다리는 image queue
		*/
		createSwapChain();
		/*
		위에서 swap chain을 생성했다. 이제 이것을 사용하려면 그 안에 
		VkImage를 위한 VkImageView(image를 액세스 하는 방법, 액세스할 이미지 부분을 기술)를 생성해야 한다.
		*/
		createImageViews();		
		//pipeline 생성 전에 Vulkan에게 렌더링 동안 사용할 framebuffer attachment에 대해 알려줘야 한다.
		// color/depth buffer가 잇는지 어떤 컨텐츠로 렌더링 작업을 처리해야하는 지 등을 render pass에 랩핑한다.
		createRenderPass();
		// 위에서 생성한 구조체, 오브젝트를 조합하여 graphic pipeline 생성!
		createGraphicsPipeline();
		/*
		attachment는 VkFramebuffer로 랩핑되서 바인딩 사용 / framebuffer는 모든 VkImageView를 참조.
		우리는 attachment가 하나지만 image는 swap chain이 반환하는 image에 의존합니다.
		이 말은 swap chain에 있는 모든 image를 위해 fraem buffer를 더 만들어야하고 
		drawing time에 획득한 이미지에 부합하는 frame buffer를 사용해야 한다는 뜻이다.
		*/
		createFramebuffers();
		// Vulkan에선 수행하고자 하는 모든 operation들을 command buffer에 기록해야 함.
		// 고로 command 사용을 위해 pool먼저 생성
		createCommandPool();
		// 이제 command buffer를 할당하고 거기에 drawing command를 기록할 수 있다.
		createCommandBuffer();
		// image를 획득되었고 렌더링할 준비가 되었다는 signal을 위해 semaphore 그리고
		// 렌더링이 완료되고 presentation이 발생할 수 있다는 signal을 위한 semaphore이 필요합니다.
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

		//frame buffer는 image view들과 render pass 이후에 삭제 되야 함.
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
		// 이벤트 심각도 지정
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		// 콜백이 호출되도록 ㅇ하는 이벤트 유형 지정
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		//콜백 함수
		createInfo.pfnUserCallback = debugCallback;
	}

	void setupDebugMessenger() 
	{
		if (!enableValidationLayers) return;

		//새로 생성된 디버그 메신저의 매개변수를 지정하는 구조
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}

	
	void createSurface() 
	{
		// surface ? 렌더링된 이미지를 표시할 곳

		//각 플랫폼별 다른 구현을 통해 Vulkan surface 생성
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

		float queuePriority = 1.0f; // 0.0 ~ 1.0 사이에서 사용 가능. 우선 순위 할당.
		for (uint32_t queueFamily : uniqueQueueFamilies) 
		{
			//Queue Family에서 우리가 원하는 queue의 개수를 기술.
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures{};

		// VkDeviceQueueCreateInfo, VkPhysicalDeviceFeatures를 통해 VkDeviceCreateInfo 생성 가능.
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

		// 각 queue family에서 queue 핸들을 찾아온다. 
		// ( logical device, queue family, queue index, queue handle 저장할 포인터)
		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
	}

	void createSwapChain() 
	{
		/*
		Swap Chain은 윈도우 surface와 호환되지 않을 수 있음으로 많은 세부 항목에 대한 질의가 필요합니다.

			1. Basic Surface capabilites(swap chain의 최대/최소 이미지 개수, 이미지 w/h 최대 최소 값)
			2. surface format(pixel format, color space)
			3. 사용 가능한 presentation 모드
		*/
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

		/*
		swap chain이 지원이 된다는 확인은 충분하다.
		이제 최적화를 확인하기 위해 3가지 타입을 설정한다.
			
			1. surface format(color depth)
			2. presentation mode(이미지를 화면에 스와핑하기 위한 조건)
			3. swap extent(swap chain의 이미지 해상도)
		*/

		// 1. surface format(color depth)
		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		// 2. presentation mode(이미지를 화면에 스와핑하기 위한 조건)
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		// 3. swap extent(swap chain의 이미지 해상도)
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);


		// swap chain이 사용할 이미지 개수도 정한다. + 1 은 일단 최소 1개 이상의 이미지를 요청하겠다는 의미이다.
		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) 
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		//이제 Vulkan 오브젝트 관례대로 swap chain의 구조체를 채워보자.
		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		// 이미지를 구성하는 layer의 양 (3D APP을 개발하지 않는 이상 항상 1임.)
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		// Graphic queue family와 Presentation queue family가 다른 경우 graphic queue에서 
		// swap chain 이미지, presentation queue에 그 이미지를 제출하게 됨.

		// Queue family간에 사용되는 S.C 이미지 핸들링 방법을 지정함.
		if (indices.graphicsFamily != indices.presentFamily) 
		{
			// 명시적인 소유권 전송 없이 이미지는 여러 Queue Family에서 사용 가능.
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else 
		{
			// 이미지를 한 Queue family에서 소유하고 다른 Q.F에서 사용하려는 경우 명시적으로 소유권 전송.
			// 이 옵션은 최상의 성능을 제공 함.
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; 
		}

		// swap chain의 trasnform( ex: 90도 회전.. 수평 플립 등). 그대로 둘거면 current 하면 됨.
		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		//윈도우 시스템에서 다른 윈도우와 블렌딩시 알파 채널 사용할 건가를 지정.
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;// 알파 채널 무시
		createInfo.presentMode = presentMode;
		// 가려진 픽셀을 신경쓰지 않겠다는 뜻
		createInfo.clipped = VK_TRUE;

		// 윈도우 리사이징할 때 등 이전 swap chain을 참조하기 위해 이 필드를 지정해야 함.
		// 복잡함으로 이것은 일단 null로 둠.(없을 시 항상 새로 생성)
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		// 모두 지정했으니 swap chain create!
		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to create swap chain!");
		}

		// 먼저 imageCount를 통해 이미지 개수를 질의한 뒤
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		// 컨테이너 크기를 조정하고
		swapChainImages.resize(imageCount);
		// 마지막으로 이를 다시 호출하여 핸들을 얻어온다. 이는 얼마든지 더 많은 수의 swapChain을 생성할 수 있기 떄문이다.
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	void createImageViews() 
	{
		// swap chain 개수에 맞게 imageViews도 리사이즈
		swapChainImageViews.resize(swapChainImages.size());


		for (size_t i = 0; i < swapChainImages.size(); i++) 
		{
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapChainImageFormat;
			// color channel을 섞을 수 있도록 해줌. (단색 텍스처를 쓴다면 모든 channel을 red로 매핑할 수도 있음.)
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			// 이미지의 용도, 어떤 부분을 액세스 해야하는지 기술
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			// 다 설정 했으니 image View create!
			if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) 
			{
				throw std::runtime_error("failed to create image views!");
			}
		}
	}

	void createRenderPass() 
	{
		//swap chain 이미지들 중 하나를 나타내는 color buffer attachment
		// color(색상) buffer(버퍼) attachment(첨부물)
		//https://www.notion.so/VkAttachmentDescription-Manual-Page-774a0dde223c41939b99b4b4f04349c9
		VkAttachmentDescription colorAttachment{};
		//color attachment의 format은 swap chain image들의 format과 동일.
		colorAttachment.format = swapChainImageFormat;
		// multisampling 관련 작업 셋팅( 아직 안 함으로 1)
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

		//attatchmnet의 데이터가 렌더링 전/후에 무엇을 할 것인지 결정
		// LOAD : 기존 attachment 유지 / CLEAR : 지우기 / DONT_CARE : 기존 컨텐츠 undefined
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		// STORE : 렌더링된 컨텐츠 저장 후 읽을 수 있음 / DONT_CARE : 렌더링 후 undefined
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		//stencil 관련 설정 (stencil 버퍼로 fragment 폐기시 사용. 여기선 사용x)
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		//Vulkan의 texture와 frame buffer는 특성 pixel format인 VkImage 오브젝트로 표현 됨.
		
		// render pass를 시작하기 전 상태의 image layout 상태 지정
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;//이전 이미지가 어떤 layout이었던 상관없단 뜻.
		// render pass가 끝났을 때 자동적으로 전환될 layout을 지정.
		// 렌더링 후에 swap chain을 통해 image를 presentation 할 것이기 때문에 아래와 같이 설정.
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		//sub pass는 하나 이상의 attachment를 참조함.
		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// 단일 render pass는 여러개의 sub pass로 구성되는데 sub pass는 
		// 이전 pass의 frame buffer 내용에 의존하는 후속 렌더링 작업입니다. (ex) post-processing)

		// 지금은 삼각형 하나 띄울거니까 단일 sub pass 유지.
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

		// attachment 배열과 sub pass를 사용하여 VkRenderPassCreateInfo 구조체를 채우고 생성 가능!
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

		// 파이프라인에 코드를 전달하기 위해 VkShaderModule 오브젝트로 랩핑 해야함.
		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		// shader를 실제 사용하기 위해 VkPipelineShaderStageCreateInfo를 통해 pipeline state로 연결 해줌.
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

		/* ☆Input Assetmbler	- raw vertex 데이터 수집
		   Vertex Shader		- 모든 vertex에게 실행, model->screen 공간으로 
		   Tessellation			- mesh 퀄리티를 올리기 위해 gemetry 세부화.(벽돌, 계단 등을 덜 평평해보이도록 사용)
		   Geometry shader		- 모든 primitive에 대해 실행되고 이를 버리거나 들어온 것 보다 더 많은 primitive를 출력 가능.
								(tessellation과 유사하지만 더 유연, 보통 글카에서 성능이 안 좋아서 요새 잘 사용 x, metal엔 이 단계 없음.)
		   ☆Resterization		- primitive를 frament로 분해. fragment는 pixel 요소로 frame buffer를 채움. 
								(화면 밖 frament 폐기, depth testing으로 인한 fragment도 폐기)
		   Fragment Shader		- 살아남은 fragment로부터 어떤 framebuffer에 쓰여질지 어떤 color, depth를 사용할지 결정.
		   ☆Color blending		- frame buffer안에 동일한 pixel로 매칭되는 여러 fragment를 혼홉한다.

		   ☆이 붙은 단계가 fixed-function : 파라메터를 통해 operation을 수정할 수 있게 해주지만 미리 정의 됨
		   나머지는 programmable.
		*/
		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		//vertex shader에 전달되는 vertex 데이터의 포맷을 기술.
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		// 데이터 간의 간격과 per-vertex인지 per-instance인지 여부 결정.
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;//optional
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;//optional

		// vertex로부터 어떤 종류의 geometry를 그릴 것이냐 primitive restart가 활성화 되었는가?
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		// viewport : image에서 frame buffer로서의 위치?크기? transformation을 정의.
		// scissor : 실제로 screen에 pixel을 그리는 영역
		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		// vertex shader의 geometry를 받아 fragment shader로 색칠할 fragment로 변환.
		// 여기서 depth testing, face culiing, scissor test 수행.
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		// true라면 near/far plane을 벗어난 fragment는 폐기되는 대신 clamp됨.
		// 이 설정은 shadow map같은 특별항 상황에서 유용. (GPU feature 활성화 필요)
		rasterizer.depthClampEnable = VK_FALSE;
		// ture시 geometry가 rasteraizer 단계 진행 x 
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		// fragment가 생성되는 방법 결정 
		// FILL : fragment로 채움 / LINE : 엣지를 선으로 그림 / POINT : vertex를 점으로 그림.
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		// fragment 선의 두께 
		rasterizer.lineWidth = 1.0f;
		// face culling 우형
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		// multi sampling 구성. anti-aliasing을 수행하기 위한 방법 중 하나.		
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		// depth와 stencil buffer를 사용하기 위해 구성.
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

		// pipeline 재생성 없이 변경할 수 있는 것들( viewport, scissor, line widht, blend constant등 )이 있는데 원한다면 그들을 채워넣어야함.
		std::vector<VkDynamicState> dynamicStates = 
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();


		//shader에서 사용되는 uniform값은 global값으로 dynamic state 와 유사하게 shader 재생성 없이 drawing 시점에서 바꿀 수 있다.
		// 이 uniform은 VkPipelineLayout 오브젝트 생성을 통해 pipeline을 생성하는 동안 지정된다.
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pushConstantRangeCount = 0;

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to create pipeline layout!");
		}

		/*
		이전에 만든 모든 구조체와 오브젝트를 조합하여 드디어 graphics pipeline 생성 가능!
			- shader stages :shader module 생성
			- Fixed-function state : pipe line의 fixed-funtion을 정의
			- pieline layout : shader에 의해 참조되는 uniform과 push 변수는 draw time에 업뎃 가능
			- render pass :pipeline 단계에 참조하는 attachment와 그 사용법
			
		이 모든 것들이 조합되어  graphics pipeline의 기능을 완전히 정의합니다.
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

		// pipeline 생성 후 지워야 함.
		vkDestroyShaderModule(device, fragShaderModule, nullptr);
		vkDestroyShaderModule(device, vertShaderModule, nullptr);
	}

	void createFramebuffers() 
	{
		// swap chain의 모든 image를 위한 frame buffer들을 저장할 멤버 설정
		swapChainFramebuffers.resize(swapChainImageViews.size());

		// imageView 개수만큼 framebuffer 생성
		for (size_t i = 0; i < swapChainImageViews.size(); i++) 
		{
			VkImageView attachments[] = 
			{
				swapChainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			//frame buffer가 호환되는 render pass 사용(동일한 개수와 타입의 attachment를 사용해야 한다는 의미)
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
		// command buffer는 queue중 하나에 제출함으로써 실행 됨. 고로 queue를 가져 옴.
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		/*
		VK_COMMAND_POOL_CREATE_TRANSIENT_BIT : 새로운 command가 매우 자주 기록.
		VK_COMMNAD_POOL_CREATE_RESET_COMMAND_BUFFER_BIT : command buffer가 개별적으로 재기록 될 수 있음.
		*/
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		// 단일 유형의 queue에 제출되야 사용 가능.
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		//command pool 생성!
		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to create command pool!");
		}
	}

	void createCommandBuffer() 
	{
		// command buffer는 vkAllocateCommandBuffers로 할당 됨.
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		// VK_COMMAND_BUFFER_LEVEL_PRIMARY : 실행을 위해 queue에 제출될 수 있지만 다른 command buffer에서 호출 x
		// VK_COMMAND_BUFFER_LEVEL_SECONDARY : 직접 실행 x, primary command buffer에서 호출 o
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

		// command buffer 기록 시작.
		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		//render pass 를 시작해서 drawing을 시작하겠다는 의미.
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		//render pass 자신
		renderPassInfo.renderPass = renderPass;
		// 바인딩 할 attachment
		renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
		// 렌더링 영역 크기 지정
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChainExtent;
		//clear color 값
		VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		// 이제 render pass가 시작 되었습니다. command들이 기록하는 모든 함수는 근들이 가지고 있는
		// vkCmd 접두사로 알아볼 수 있습니다.
		// 첫 파라미터 : 항상 command를 기록 할 command buffer
		// 두번째 파라미터 : render pass 세부 항목
		// 세번쨰 파라미터 : render pass내에서 drawing command가 어케 제공되는지 제어.
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		//graphics pipeline 바인딩
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
			draw하기 위해 해야하는 것.

			1. swap chain으로 부터 image 획득
			2. frame buffer에서 해당 image를 attachment로 command buffer 실행
			3. presentation을 위해 swap chain에 image 반환.
		*/
		vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
		vkResetFences(device, 1, &inFlightFence);

		//1. swap chain으로 부터 image 획득
		uint32_t imageIndex;
		vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

		vkResetCommandBuffer(commandBuffer, /*VkCommandBufferResetFlagBits*/ 0);
		recordCommandBuffer(commandBuffer, imageIndex);

		// queue submit(제출) 및 동기화
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;


		//semaphore들이 실행이 시작되기 전에 기다려아 하는지, pipeline의 stage(들)을 기다려야하는지 지정.
		VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		// 2. frame buffer에서 해당 image를 attachment로 command buffer 실행
		// swap chain image를 color attachment로 바인딩하는 command buffer 제출.
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		//실행이 완료 됐을때 signal보낼 semaphore.
		VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		//위에서 셋팅했던 것들로 graphics queue에 command buffer 제출 가능.
		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		// 3. presentation을 위해 swap chain에 image 반환.
		// frame을 drawing하는 마지막 단계.
		// 결과를 swap chain에게 다시 제출하여 최종적으로 화면에 표시하는 것이다.
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		// presentatin이 발생하기 전까지 기다릴 semaphore 지정
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		//image를 표시할 swap chain들과 각 swap chain의 index
		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1; //항상 1
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;

		// swap chain에게 image를 표시하라는 요청 제출!!
		vkQueuePresentKHR(presentQueue, &presentInfo);
	}

	VkShaderModule createShaderModule(const std::vector<char>& code) 
	{
		//  VkShaderModule 오브젝트로 shader 랩핑.
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
			
		//1. Basic Surface capabilites(swap chain의 최대 / 최소 이미지 개수, 이미지 w / h 최대 최소 값)
		// 지정된 Physical Device(GPU)와 윈도우 Surface를 사용하여 지원되는 capability를 결정함.
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		//2. surface format(pixel format, color space)
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

		if (formatCount != 0) 
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		//3. 사용 가능한 presentation 모드
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
		// queue family 리스트를 얻어옴
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies) 
		{
			//VK_QUEUE_GRAPHICS_BIT 를 지원하는 최소 하나의 queue family를 찾아야 함.
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