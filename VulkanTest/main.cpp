
#define GLFW_INCLUDE_VULKAN
#include "vulkan/vulkan.h"
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

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

	void initWindow()
	{
		//glfw 초기화
		glfwInit();

		//glfw는 원래 openGL context를 생성하게 돼있기 때문에 openGL context를 생성하지 않도록 지시해야한다.
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// 당장은 리사이즈 처리가 어렵기 때문에 resize flase
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	}

	/**
	 * @brief 
	 */
	void initVulkan()
	{
		createInstance();
	}

	void mainLoop()
	{
		while (!glfwWindowShouldClose(window)) 
		{
			glfwPollEvents();
		}
	}

	void cleanup()
	{
		vkDestroyInstance(instance, nullptr);

		glfwDestroyWindow(window);

		glfwTerminate();
	}

	void createInstance()
	{
		// application에 대한 정보
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		// Vulkan 드라이버에 사용할 전역 확장 및 유효성 검사 layer
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		// VILKAN은 플랫폼에 구애받지 않는 API다. 즉 window system과 interface를 위해 확장이 필요하다.
		// global extensions을 정한다.
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		createInfo.enabledExtensionCount = glfwExtensionCount;
		createInfo.ppEnabledExtensionNames = glfwExtensions;
		// 활성화할 전역 유혀성 검사 layer를 결정하지만 일단 비워둔다.
		createInfo.enabledLayerCount = 0;

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
	}
};

int main()
{
	HelloTriangleApplication app;

	try 
	{
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}