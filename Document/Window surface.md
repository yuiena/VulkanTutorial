- Window Surface 생성
- Presentation 지원 요청
- Presentation Queue 생성



Vulkan은 플랫폼에 구애받지 않는 API이므로 자체적으로 Window System과 직접 연동 할 수 없습니다. 

Vulkan과 Window System 간의 연결을 설정하여 결과를 화면에 표시하려면 WSI(Window System Integration)을 확장을 사용해야 합니다. 

이 장에서는 첫 번째 항목인 `VK_KHR_surface`에 대해 살펴보겠습니다.  렌더링된 이미지를 표시할 Surface의 추상 유형을 나타내는`VkSurfaceKHR` 개체를 노출합니다 . 우리 프로그램의 Surface은 GLFW로 이미 연 Window에 의해 뒷받침됩니다.

확장은 `VK_KHR_surface`인스턴스 수준 확장이며 에서 반환된 목록에 포함되어 있기 때문에 실제로 이미 활성화했습니다 `glfwGetRequiredInstanceExtensions`. 목록에는 다음 몇 장에서 사용할 다른 WSI 확장도 포함되어 있습니다.

Window surface은 실제로 Physical Device 선택에 영향을 줄 수 있으므로 인스턴스 생성 직후에 생성해야 합니다. 

우리가 이것을 연기한 이유는 Window Surface이 Render Target 및 Presentation의 더 큰 주제의 일부이기 때문에 설명이 기본 설정을 복잡하게 만들 수 있기 때문입니다. 

또한 화면 외 렌더링이 필요한 경우 Window Surface은 Vulkan에서 완전히 선택적인 구성 요소입니다. Vulkan을 사용하면 보이지 않는 Window을 만드는 것과 같은 해킹 없이 이를 수행할 수 있습니다(OpenGL에 필요).



# Window Surface 생성

디버그 콜백 바로 아래에 `surface`클래스 멤버 를 추가하여 시작하십시오 .

```c++
VkSurfaceKHR surface;
```

`VkSurfaceKHR`객체와 그 사용법은 플랫폼에 구애받지 않지만  Window System  세부 정보에 의존하기 때문에 생성되지 않습니다. 

예를 들어 Windows에서는 `HWND`및 `HMODULE`핸들이 필요합니다. 

`VK_KHR_win32_surface`따라서 Windows에서 호출 되고 의 목록에도 자동으로 포함 되는 `glfwGetRequiredInstanceExtensions`확장에 플랫폼별 추가 사항이 있습니다 .

이 플랫폼별 확장을 사용하여 Windows에서 Surface을 만드는 방법을 보여주겠지만 이 자습서에서는 실제로 사용하지 않습니다. 

GLFW와 같은 라이브러리를 사용한 다음 플랫폼별 코드를 계속 사용하는 것은 의미가 없습니다. 

GLFW에는 실제로 플랫폼 차이를 처리하는 `glfwCreateWindowSurface`기능이 있습니다. 그럼에도 불구하고, 우리가 그것에 의존하기 시작하기 전에 그것이 뒤에서 무엇을 하는지 보는 것이 좋습니다.

기본 플랫폼 기능에 액세스하려면 상단의 include를 업데이트해야 합니다.

```c++
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
```

Window Surface는 Vulkan 개체이기 때문에 `VkWin32SurfaceCreateInfoKHR`채워야 하는 구조체가 함께 제공됩니다. 

여기에는 두 가지 중요한 매개변수가 있습니다. `hwnd`및 `hinstance`. 이것들은 Window과 Process에 대한 Handle입니다.

```c++
VkWin32SurfaceCreateInfoKHR createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
createInfo.hwnd = glfwGetWin32Window(window);
createInfo.hinstance = GetModuleHandle(nullptr);
```

`glfwGetWin32Window`함수는 GLFW Window 개체에서 raw `HWND`를 가져오는 데 사용 됩니다 . 

`GetModuleHandle`호출은 현재 프로세스의 `HINSTANCE`핸들을 반환 합니다 .

그 후 `vkCreateWin32SurfaceKHR`인스턴스에 대한 매개변수, Surface 생성 세부 정보, 사용자 지정 할당자 및 Surface Handle 이 저장될 변수를 포함하는 를 사용하여 Surface을 생성할 수 있습니다. 

기술적으로 이것은 WSI 확장 함수이지만 너무 일반적으로 사용되므로 표준 Vulkan 로더에 포함되어 있으므로 다른 확장과 달리 명시적으로 로드할 필요가 없습니다.

```c++
if (vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface!");
}
```

Process는 Linux와 같은 다른 플랫폼과 유사합니다. 여기서 `vkCreateXcbSurfaceKHR`XCB 연결 및 창을 X11의 생성 세부 정보로 사용합니다.

이 `glfwCreateWindowSurface`기능은 플랫폼마다 다른 구현으로 정확히 이 작업을 수행합니다. 이제 프로그램에 통합할 것입니다. 

인스턴스 생성 직후`initVulkan`에서 호출할  `createSurface`함수를 추가 하고 `setupDebugMessenger`를 추가합니다.

```c++
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
}

void createSurface() {

}
```

GLFW 호출은 함수 구현을 매우 간단하게 만드는 구조체 대신 간단한 매개변수를 사용합니다.

```c++
void createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}
```

매개변수는 [`VkInstance`](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkInstance.html), GLFW Window 포인터, 사용자 지정 할당자 및 `VkSurfaceKHR`변수에 대한 포인터입니다. 

단순히 [`VkResult`](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkResult.html)관련 플랫폼 호출에서 전달됩니다. GLFW는 Surface을 Destroy하는 특별한 기능을 제공하지 않지만 원래 API를 통해 쉽게 수행할 수 있습니다.

````c++
void cleanup() {
        ...
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
        ...
    }
````

인스턴스 전에 Surface이 파괴되었는지 확인하십시오.



# Presentation 지원 요청

Vulkan 구현이 Window System Integration을 지원할 수 있지만 시스템의 모든 장치가 이를 지원한다는 의미는 아닙니다. 

따라서 `isDeviceSuitable`장치가 우리가 만든 Surface에 Image를 표시할 수 있도록 확장해야 합니다. 

Presentation은 Queue 관련 기능이므로 실제로 문제는 우리가 만든 Surface에 Presentation을 지원하는 Queue Family를 찾는 것입니다.

그리기 Command을 지원하는 Queue Family과 Presentation을 지원하는 Queue Family이 겹치지 않을 수도 있습니다. 

`QueueFamilyIndices`따라서 구조를 수정하여 고유한 Presentation Queue이 있을 수 있다는 점을 고려해야 합니다 .

```c++
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};
```

다음으로, Window Surface에 표시할 수 있는 기능이 있는 Queue Family를 찾도록 `findQueueFamilies`함수를 수정합니다. 

이를 확인하는 함수 `vkGetPhysicalDeviceSurfaceSupportKHR`는 Physical Device, Queue Family 인덱스 및 Surface을 매개변수로 사용하는 입니다. 

다음과 같은 루프에서 호출을 추가합니다 `VK_QUEUE_GRAPHICS_BIT`.

```c++
VkBool32 presentSupport = false;
vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
```

그런 다음 단순히 Bool 값을 확인하고 Presentation Queue Family인덱스를 저장합니다.

```
if (presentSupport) {
    indices.presentFamily = i;
}
```

이들은 결국 동일한 Queue Family 가 될 가능성이 매우 높지만 프로그램 전체에서 균일한 접근을 위해 별도의 Queue인 것처럼 취급할 것입니다. 그럼에도 불구하고 성능 향상을 위해 동일한 Queue에서 그리기 및 Presentation을 지원하는 Physical Device를 명시적으로 선호하는 논리를 추가할 수 있습니다.



# Presentation Queue 생성

남은 한 가지는 논리적 장치 생성 절차를 수정하여 Presentation Queue를 만들고 [`VkQueue`](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkQueue.html)핸들을 검색하는 것입니다. 핸들에 대한 멤버 변수를 추가합니다.

```
VkQueue presentQueue;
```

[`VkDeviceQueueCreateInfo`](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkDeviceQueueCreateInfo.html)다음으로, 두 Family에서 Queue  를 생성하려면 여러 구조체 가 필요합니다 . 이를 위한 우아한 방법은 필요한 Queue에 필요한 모든 고유한  Queue Family  세트를 만드는 것입니다.

```c++
#include <set>

...

QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

float queuePriority = 1.0f;
for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
}
```

[`VkDeviceCreateInfo`](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkDeviceCreateInfo.html)벡터를 가리키도록 수정 합니다.

```c++
createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
createInfo.pQueueCreateInfos = queueCreateInfos.data();
```

 Queue Family 가 동일한 경우 해당 인덱스를 한 번만 전달하면 됩니다. 마지막으로 Queue Handle을 검색하는 호출을 추가합니다.

```
vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
```

 Queue Family 가 동일한 경우 두 Handle이 이제 동일한 값을 가질 가능성이 높습니다. 다음 장에서는 Swap Chain과 이들이 Surface에 이미지를 표시하는 기능을 제공하는 방법을 살펴볼 것입니다.

[C++ 코드](https://vulkan-tutorial.com/code/05_window_surface.cpp)

















