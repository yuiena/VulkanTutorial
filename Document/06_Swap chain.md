Vulkan에는 "default framebuffer"라는 개념이 없습니다. 그러므로 우리가 Screen에 시각화하기 전에 렌더링할 버퍼를 소유하기 위한 인프라구조가 필요합니다. 이 인프라구조를 ***swap chain*** 이라고 하며 Vulkan에서 명시적으로 생성해야 합니다. 

swap chain은 본질적으로 "스크린에 출력되기 전에 기다리는 이미지(waiting image)"의 queue 입니다. 

우리 어플리케이션은 그와 같은 이미지(스크린에 출력되기 전에 기다리는 이미지)를 가져와서 그 이미지를 queue에 반환합니다. queue의 정확한 작동 방식과 queue에 있는 이미지를 출력하는 조건은 swap chain의 설정에 따라 다르겠지만, swap chain의 일반적인 목적은 `이미지의 출력를 화면 새로고침 빈도와 동기화` 하는 것입니다.

## **Checking for swap chain support**

여러가지 이유로 모든 그래픽카드가 직접 화면에 이미지를 출력하는 기능을 가지진 않습니다. 예를 들어 서버를 위해 디자인 되어서 디스플레이 출력이 없을 수도 있습니다. 두번째로 이미지 출력은 Window System과 Window와 연결된 surface에 밀접하게 연결되어 있습니다. 이건 실제로 Vulkan 코어의 일부가 아닙니다. 우리는 **`VK_KHR_swapchain`**이 디바이스에서 지원되는지를 쿼리한 후에 이 디바이스 extension을 활성화 해야 합니다.

이 목적(`VK_KHR_swapchain` extension이 지원되는지 확인)으로 우리는 **`isDeviceSuitable`** 함수를 확장할 겁니다. 우리는 이전에 **[VkPhysicalDevice](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkPhysicalDevice.html)**를 사용하여 지원되는 extension을 나열하는 방법을 봤었습니다. 따라서 이 작업은 매우 간단합니다. Vulkan 헤더에는 **`VK_KHR_SWAPCHAIN_EXTENSION_NAME`**라는 멋진 매크로가 제공되는데 이는 **`VK_KHR_swapchain`** 문자열을 정의합니다. 이 매크로 사용의 이점은 컴파일러에서 오타를 포착할 수 있다는 점입니다.

먼저 필요한 디바이스 extension 리스트를 정의합니다. validation layer를 활성화 하기 위해 사용했던 리스트와 비슷합니다.

```cpp
const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
```

다음으로 추가적인 체크를 위해 **`checkDeviceExtensionSupport`** 함수를 생성하고 이를 **`isDeviceSuitable`**에서 호출합니다.

```cpp
bool isDeviceSuitable(VkPhysicalDevice device) {
	QueueFamilyIndices indices = findQueueFamilies(device);

	bool extensionsSupported = checkDeviceExtensionSupport(device);

	return indices.isComplete() && extensionsSupported;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
	return true;
}
```

함수 바디를 수정하여 extension을 나열하고 필요한 모든 extension이 포함되어 있는지 체크합니다.

```cpp
bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
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
```

확인되지 않은 필수 extension의 표시하기 위해서 여기서 `set<string>`을 사용하였습니다. 이렇게 하면 사용가능한 extension을 순서대로 나열하면서 쉽게 해당 extension을 체크할 수 있습니다. 물론 **`checkValidationLayerSupport`**에서 처럼 중첩 루프를 사용해도 됩니다. 성능 차이는 별로 없을 겁니다. 

이제 코드를 실행하고 우리 그래픽 카드가 실제 swap chain을 생성할수 있는지를 검증하십시오.  주목할 점은 이전 챕터에서 우리가 체크했었던 presentation queue 가용성이 swap chain extension이 지원되고 있음을 암시한다는 것입니다. 그러나 그 부분에 대해서 명확하게 하는 것은 언제나 좋으며  extension은 명시적으로 활성화 되어야 합니다.



## **Enabling device extensions**

swapchain을 사용하기 위해서는 먼저 `VK_KHR_swapchain` extension을 활성화해야 합니다. extension을 활성화 하기 위해선 논리 디바이스 생성 구조체를 약간 수정할 필요가 있습니다.

```cpp
createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
createInfo.ppEnabledExtensionNames = deviceExtensions.data();
```

이 경우 기존 라인 **`createInfo.enabledExtensionCount = 0;`**을 교체해야 합니다.



## **Querying details of swap chain support**

swap chain을 사용할 수 있는지 체크하는 것 만으로는 부족합니다. 이 swap chain이 우리의 윈도우 surface와 실제 호환되지 않을 수도 있기 때문입니다. 

또한 swap chain을 생성하는 것은 instance나 device 생성보다 더 많은 설정을 수반하므로 이 작업을 수행하기 전에 몇가지 세부 항목에 대한 질의가 필요합니다.

여기 기본적으로 체크해야 하는 3가지의 속성이 있습니다.



- Basic surface capabilities (swap chain의 최대/최소 이미지 갯수, 이미지 너비/높이의 최대/최소 값)

- Surface format (pixel format, color space)

- 사용 가능한 presentation 모드

  

**`findQueueFamilies`**와 비슷하게,  구조체를 사용하여 질의한 후 이러한 세부정보를 전달할 겁니다. 앞서 언급한 세가지 유형의 속성은 아래 구조체와 구조체 리스트 형태로 제공됩니다.

```cpp
struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};
```

이제 구조체에 데이터를 채울 새로운 함수인 **`querySwapChainSupport`** 를 생성하겠습니다.

```cpp
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
	SwapChainSupportDetails details;
	return details;
}
```

이 섹션에서는 해당 정보를 포함한 구조체를 질의하는 방법을 설명합니다. 이 구조체와 그들이 가지고 있는 데이터의 정확한 의미는 다음 섹션에서 설명하겠습니다.

Basic surface capabilities 부터 시작해 보겠습니다. 이 속성은 간단하게 질의 가능하고 단일 **`VkSurfaceCapabilitiesKHR`** 구조체를 반환합니다.

```cpp
vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
```

이 함수는 지정된 **[VkPhysicalDevice](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkPhysicalDevice.html)**와 **`VkSurfaceKHR`** 윈도우 surface를 사용하여 지원되는 capability를 결정합니다. 

모든 지원 질의 함수는 이 두개의 인자를 첫번째 파라미터로 가집니다. 왜냐면 이 두개가 swap chain의 핵심 구성요소이기 때문입니다.

다음 단계는 지원 포맷의 질의에 대한 것입니다. 이것은 구조체 리스트이기 때문에  2개의 함수 호출의 익숙한 패턴을 따릅니다.

```cpp
uint32_t formatCount;
vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

if (formatCount != 0) {
	details.formats.resize(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
}
```

벡터가 모든 가능한 포맷을 저장할 수 있도록 resize 된 것을 확인하십시오. 마지막으로 지원되는 presentation mode를 질의하는 것은 **`vkGetPhysicalDeviceSurfacePresentModesKHR`**를 사용하여 위와 완벽하게 같은 방식으로 작업합니다.

```cpp
uint32_t presentModeCount;
vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

if (presentModeCount != 0) {
	details.presentModes.resize(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
}
```

이제 모든 세부항목들이 구조체에 들어갔습니다. 이제 **`isDeviceSuitable`**을 한번 더 확장하여 이 함수를 swap chain 지원에 알맞는지 검증하는데 사용합니다.  우리가 가지고 있는 윈도우 surface에서 최소 한개 이상의 지원되는 이미지 포맷과 지원되는 한개의 presentation mode만 주어지더라도, 이 튜토리얼에서의 swap chain 지원으로 충분합니다.

```cpp
bool swapChainAdequate = false;
if (extensionsSupported) {
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
	swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
}
```

extension을 사용할 수 있는지를 확인한 후에 swap chain 지원을 질의하는 것이 중요합니다. 함수의 마지막 라인은 아래와 같이 수정합니다.

```cpp
return indices.isComplete() && extensionsSupported && swapChainAdequate;
```

## **Choosing the right settings for the swap chain**

**`swapChainAdequate`** 조건이 충족되면 swap chain이 지원에 대한 확인은 확실히 충분합니다. 그렇지만 여전히 다양한 최적화 모드가 있을 수 있습니다.  우리는 이제 가능한 최적의 swap chain의 올바른 설정을 찾기 위한 두서너개의 함수를 작성할 것입니다. 여기 결정해야 할 3가지 타입의 설정이 있습니다.

- Surface format (color depth)
- Presentation mode (이미지를 화면에 "스와핑" 하기 위한 조건)
- Swap extent (swap chain의 이미지의 해상도)

이 각각의 설정들에 대해 이상적인 값을 고려할 것입니다. 이를 위해 만약 해당 설정들이 가능하면 그걸로 가고, 그렇지 않다면 차선책을 찾는 임의의 로직을 생성할 것입니다.



### **Surface format**

이 설정 함수는 다음과 같이 시작합니다. 우린 나중에 **`SwapChainSupportDetails`**의 **`formats`** 멤버를 인자로 전달할 겁니다.

```cpp
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
}
```

각 **`VkSurfaceFormatKHR`** 항목은 **`format`** 과 **`colorSpace`** 멤버를 가지고 있습니다. **`format`** 멤버는 color channel과 타입을 지정합니다. 

예를 들어 **`VK_FORMAT_B8G8R8A8_UNORM`**은 각 color channel이 B,G,R,A의 순서대로 되어 있으며 픽셀당 8bit unsigned integer 형식으로 총 32bit로 저장되어 있다는 의미입니다.

 **`colorSpace`** 멤버는 **`VK_COLOR_SPACE_SRGB_NONLINEAR_KHR`** 플래그를 사용하여 SRGB color space를 지원하고 있는지 여부를 나타냅니다. 주의 : 이전 버전 스펙에서 이 플래그는  **`VK_COLORSPACE_SRGB_NONLINEAR_KHR`**이었습니다.

만약 SRGB가 사용 가능하면 이를 color space에 사용할 것입니다. 왜나면 **[SRGB의 결과가 색상을 좀 더 정확하게 인식](http://stackoverflow.com/questions/12524623/)**할 수 있기 때문입니다. 또한 이것은 우리가 나중에 사용하게될 텍스쳐 같은 이미지의 표준 color space입니다. 그 때문에도 우리는 SRGB color format을 사용해야 하고 그 중 가장 일반적인 형식중 하나는 `VK_FORMAT_B8G8R8A8_SRGB`입니다.

목록을 살펴보고 선호하는 조합을 사용할 수 있는지 확인합시다.

```cpp
for (const auto& availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        return availableFormat;
    }
}
```

이것도 실패하면, "good" 정도에 따라 사용 가능한 포맷의 순위를 지정할 수도 있지만,대부분의 경우 지정된 첫번째 형식으로 해결하는 것이 좋습니다.

```cpp
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}
```

### **Presentation mode**

presentation mode는 swap chain을 위한 반론의 여지 없는 가장 중요한 설정입니다. **이미지를 출력하기 위한 실제적인 조건**을 나타내기 때문입니다. 여기에 Vulkan에서 지원하는 4개의 가능한 mode가 있습니다.

- **`VK_PRESENT_MODE_IMMEDIATE_KHR`** : 어플리케이션에서 제출된 이미지는 즉시 화면에 전송됩니다. **[Tearing 현상](http://bozeury.tistory.com/entry/Tearing-%ED%98%84%EC%83%81%EA%B3%BC-%EC%88%98%EC%A7%81%EB%8F%99%EA%B8%B0%ED%99%94)**이 발생할 수 있습니다.
- **`VK_PRESENT_MODE_FIFO_KHR`** : swap chain은 queue입니다. 디스플레이가 Refresh 되고 프로그램이 렌더링 된 이미지들을 queue의 뒤에 삽입할 때 디스플레이가 queue의 앞에서 이미지를 가져오게 됩니다. 만약 queue가 가득차면 프로그램은 대기하게 됩니다. 이것은 현대 게임에서 볼 수 있는 수직 동기화(verical sync)와 가장 유사합니다. 디스플레이가 새로고쳐지는 순간을 "수직 공백(vertical blank)"라고 합니다.
- **`VK_PRESENT_MODE_FIFO_RELAXED_KHR`** : 이 모드는 어플리케이션이 지연되고 마지막 수직 공백 시점에 queue가 비어있는 상태에서만 이전 모드와 다릅니다. 이미지가 도착하면 다음 수직 공백을 기다리는 대신 즉시 전송됩니다. 이 결과 **[tearing 현상](http://bozeury.tistory.com/entry/Tearing-%ED%98%84%EC%83%81%EA%B3%BC-%EC%88%98%EC%A7%81%EB%8F%99%EA%B8%B0%ED%99%94)**이 발생할 수 있습니다.
- **`VK_PRESENT_MODE_MAILBOX_KHR`** : 이 모드는 두번째 모드의 또 다른 변형입니다. queue가 꽉차 있을 때 어플리케이션을 차단(blocking)하는 대신 이미 queue에 있는 이미지가 간단히 새로운 이미지로 교체됩니다. 이 모드는 triple buffering을 구현할 때 사용할 수 있습니다. double buffering을 사용하는 표준 수직 동기화보다 대기 시간 이슈(latency issue)가 훨씬 적어서 **[tearing 현상](http://bozeury.tistory.com/entry/Tearing-%ED%98%84%EC%83%81%EA%B3%BC-%EC%88%98%EC%A7%81%EB%8F%99%EA%B8%B0%ED%99%94)**을 피할 수 있습니다. 이것은 일반적으로 "triple buffering"으로 알려져 있지만, 3개의 버퍼가 있다고 반드시 framerate가 unlocked 되는 것은 아닙니다.



> `VK_PRESENT_MODE_MAILBOX_KHR` 설명이 잘못되어 있다고 합니다. present mode는 double buffering, triple buffering과 전혀 관계가 없습니다. swapchain이 queue를 관리하는 방법 그 자체만 설명하고 있는 것입니다. 
> `VK_PRESENT_MODE_MAILBOX_KHR`의 특징은 swapchain에 단 하나의 대기 슬롯만 존재한다는 것입니다. 
> 해당 대기 슬롯에 대기중인 이미지가 있을때 swapchain에 새로운 이미지를 제출하면 기존 이미지는 새로운 이미지로 대체됩니다. [[참조1](https://www.notion.so/Vulkan-Mobile-Best-Practice-How-To-configure-Your-Vulkan-Swapchain-581c617bf5dd4f60bd7048da39f1d8df)][[참조2](https://www.notion.so/VkPresentModeKHR-Manual-Page-67f6dc29251c4238b140e09f5512d9f5)]




Vulkan은 오직 **`VK_PRESENT_MODE_FIFO_KHR`**만 개런티하기 때문에 가능한 최선의 모드를 찾는 함수를 다시 작성해야 합니다.

```cpp
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {
	return VK_PRESENT_MODE_FIFO_KHR;
}
```

전력 사용(energy usage)가 문제가 되지 않는다면, 개인적으로 `VK_PRESENT_MODE_MAILBOX_KHR`이 아주 좋은 절충안이라고 생각합니다. 이는 수직 공백이 생길 때 까지 가능한 최신의 이미지를 렌더링하여 상당히 낮은 지연시간을 유지하고 이로인해 tearing 현상을 피할수 있도록 해줍니다.  전력 사용이 좀 더 중요한 mobile device에서는 대신에 `VK_PRESENT_MODE_FIFO_KHR`을 사용하길 원할 겁니다. 이제  `VK_PRESENT_MODE_MAILBOX_KHR`가 사용가능한지 리스트를 살펴봅시다.

```cpp
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}
```



### **Swap extent**

이제 주요 속성은 하나만 남았습니다. 이를 위해 우리는 마지막으로 함수 하나를 추가하겠습니다.

```cpp
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
}
```

Swap extent는 swap chain 이미지의 해상도이고 우리가 픽셀 단위(자세한 내용은 잠시 후)로 그리려고 하는 윈도우의 해상도와 거의 항상 정확히 같습니다. 가능한 해상도의 범위는 **`VkSurfaceCapabilitiesKHR`** 구조체에 정의되어 있습니다. Vulkan은 **`currentExtent`** 멤버에서 `width`와 `height`을 설정하여 윈도우의 해상도를 일치시키라고 합니다. 하지만 일부 윈도우 매니져는 여기서 다른것을 허용합니다. 

특별하게 정의된 값(**`uint32_t`**의 최대값)으로 **`currentExtent`**의 `width`와 `height`를 설정함으로써 지정할 수 있습니다. 이런 경우 **`minImageExtent`**와 **`maxImageExtent`** 범위에서 윈도우와 가장 일치하는 해상도를 선택해야 합니다. 하지만, 올바른 단위로 해상도를 지정해야 합니다.

GLFW는 size를 측정할 때 두가지 단위를 사용합니다: pixel과 [screen coordinate](https://www.glfw.org/docs/latest/intro_guide.html#coordinate_systems). 예를 들어, 앞서 윈도우를 생성할 때 지정했던 해상도 `{WIDTH, HEIGHT}`는 screen coordinate 단위로 측정됩니다. 

하지만 Vulkan은 pixel을 사용해서 작업하므로 swap chain extent는 **pixel 단위도 지정**되어야 합니다. 불행히도, 여러분이 high DPI display(apple의 Retina display 같은)을 사용하는 경우, screen coordinate는 pixel과 일치하지 않습니다. 대신, 높은 pixel 밀도 때문에 pixel 단위의 윈도우 해상도는 screen coordinate 단위의 해상보다 클 것입니다. 따라서 Vulkan이 우리를 위해 swap extent를 고치지(fix) 않으면, 원래의 `{WIDTH, HEIGHT}`를 사용할 수 없습니다. 대신 최대/최소 image extent를 매칭하기 전에 pixel 단위로 윈도우의 해상도를 쿼리하기 위해 `glfwGetFramebufferSize`를 사용해야 합니다.

```cpp
#include <cstdint> // Necessary for UINT32_MAX
#include <algorithm> // Necessary for std::min/std::max

...

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
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
```

여기서 `clamp` 함수는 구현에 의해 지원되는 허용되는 최대/최소 extent 사이에서 `width`와 `height` 값을 제한하는데 사용됩니다.



## **Creating the swap chain**

이제 우리는 런타임에 해야하는 선택을 지원해줄 모든 헬퍼 함수를 만들었습니다. 이제 최종적으로 swap chain 생성 작업에 필요한 모든 정보를 갖게 되었습니다.

**`createSwapChain`** 함수를 만들고 이 헬퍼함수들의 결과로 부터 시작하게 합니다. **`initVaulkan`** 함수의 논리 디바이스 생성 이후에 `createSwapChain`을 호출해야 합니다.

```cpp
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
}

void createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
}
```

이들 프로퍼티 외에도 swap chain에 포함할 이미지수도 결정해야 합니다. implementation에서는 작동에 필요한 최소 수를 지정합니다.

```cpp
uint32_t imageCount = swapChainSupport.capabilities.minImageCount;
```

하지만, 단순히 이 최소치를 유지한다는 것은 우리가 때때로 렌더링을 위해 다른 이미지를 요청할 수 있기 전에 driver가 내부 operation을 완료할 때 까지 대기해야 한다는 것을 의미합니다. 따라서 최소치 보다 최소 1개 이상의 이미지을 요청하는 걸 권장합니다.

```cpp
uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
```



>이게 많이 애매한 말입니다. swapchain에서 활용하는 이미지의 개수를 지정하는 것인데, 우리가 선택한 presentation mode에 따라 필요한 이미지의 갯수가 다르기 때문입니다. 
>
>만약 present mode가 MAILBOX라면 내부에 대기할 수 있는 하나 뿐인지라 최소 2개(display, queued)나 3개(displayed, queued, rendering)로 사용할 수 있고 그 이상은 일반적으로 별 의미가 없을 것입니다. 만약 minImageCount가 3으로 넘어올 경우 여기에 + 1 한다는게 무슨 의미가 있을까요?  `imageCount`는 그냥 `minImageCount + 1` 하는게 아니라 `minImageCount`와 현재 present mode를 보고 확인하게 맞습니다.
>[VK_KHR_swapchain manual page](https://www.notion.so/VK_KHR_swapchain-Manual-Page-3d778d3ffeee42619048cebb51553b5c)의 이슈 12에서 이 `minImageCount + 1`에 대한 이야기가 나오는데요. 기본적으로 MAILBOX mode에 대한 이야기 이고, minImageCount = presentation engin에서 사용하는 image 수(둘 이상의 current 이미지를 가질 수 있다고 함) + 대기 슬롯 이미지 수(MAILBOX는 1개)라고 가정했다고 생각합니다. 이 경우 `minImageCount + 1`은 어플리케이션이 현재 사용중인 swapchain image가 없을 때 block 없이 image를 얻을 수 있는 최소 수가 됩니다. 무조건적이지 않기 때문에 항상 minImageCount와 present mode를 확인해서 결정해야 합니다.



이것을 하는 동안 이미지의 최대 수를 넘어가지 않게 확인하는 것도 필요합니다. 여기서 `0`은 특별한 값으로 최대치가 없다는 뜻입니다.

```cpp
if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) 
{
	imageCount = swapChainSupport.capabilities.maxImageCount;
}
```

Vulkan 오브젝트의 관례대로 swap chain 오브젝트를 생성하기 위해서는 큰 구조체를 채워야 합니다. 이젠 매우 익숙할겁니다.

```cpp
VkSwapchainCreateInfoKHR createInfo = {};
createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
createInfo.surface = surface;
```

swap chain에 연결해야할 surface를 지정한 뒤, swap chain image에 대한 세부사항을 지정합니다.

```cpp
createInfo.minImageCount = imageCount;
createInfo.imageFormat = surfaceFormat.format;
createInfo.imageColorSpace = surfaceFormat.colorSpace;
createInfo.imageExtent = extent;
createInfo.imageArrayLayers = 1;
createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
```

**`imageArrayLayers`**는 이미지를 구성하는 layer의 양을 지정합니다. 이것은 입체 3D 어플리케이션을 개발하지 않는 한 항상 **1**입니다. **`imageUsage`** 비트 필드는 swap chain에 있는 이미지를 위해 어떤 종류의 operation를 사용할지 지정합니다. 이 튜토리얼에서 우리는 이 이미지에 다이렉트로 렌더링 할 것이고 그 의미는 이 이미지들을 color attachment로 사용하겠다는 것입니다. 

Postprocessing 같은 operation을 수행하기 위해서 이미지를 먼저 별도의 이미지에 렌더링 하는 것도 가능합니다. 이 경우 **`VK_IMAGE_USAGE_TRANSFER_DST_BIT`**같은 값을 대신 사용할 것이고 메모리 operation을 이용하여 렌더링 된 이미지를 swap chain 이미지에 전송할 것입니다.

```cpp
QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

if (indices.graphicsFamily != indices.presentFamily) 
{
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
} 
else 
{
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0; // Optional
    createInfo.pQueueFamilyIndices = nullptr; // Optional
}
```

그다음 여러 queue family 간에서 사용되는 swap chain 이미지를 핸들링하는 방법을 지정해야 합니다. 

우리 어플리케이션에서 보면 **graphic queue family**와 **presentation queue family**가 다른 경우입니다. 



> **presentation queue family** : Presentation은 아마도 Rendering 된 이미지를 Display할 때 사용되는 것으로 추측된다. Vulkan API를 사용해서 이미지를 Rendering 하는 경우 Presentation Queue도 생성해야 한다.
>
> Rendering이 완료된 이미지를 Display 하는 과정에서 필요한 Command를 전달하는 Queue가 아닐까 추측해본다.



graphic queue에서 swap chain 이미지를 그리고 presentation queue에 그 이미지를 제출하게 될겁니다. 여러 queue에서 엑세스하는 이미지를 핸들링하기 위한 방법이 두가지 있습니다.

- **`VK_SHARING_MODE_EXCLUSIVE`** : 한 시점에 이미지는 하나의 queue family가 소유하고 다른 queue family에서 그 이미지를 사용하려 할 경우 그전에 명시적으로 소유권을 전송해야 합니다. 이 옵션은 최상의 성능을 제공합니다.
- **`VK_SHARING_MODE_CONCURRENT`** : 명시적인 소유권 전송 없이 이미지는 여러 queue family에서 사용될 수 있습니다.

만약 queue family가 다르다면 이 튜토리얼에서는 소유권 전송을 피하기 위해 concurrent mode를 사용할 겁니다. 

소유권 전송에는 나중에 더 잘 설명하게 될 몇가지 개념이 포함되어 있기 때문입니다. Concurrent mode는 **`queueFamilyIndexCount`**와 **`pQueueFamilyIndices`** 파라미터를 사용하여 소유권을 공유할 queue family를 사전에 지정해야 합니다. 

graphics queue family와 presentation queue family가 같다면 (대부분의 하드웨어서 같습니다) exculsive mode로 고정합니다. concurrent mode는 적어도 두개의 별개의 queue family를 지정해야 하기 때문입니다.

```cpp
createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
```

swap chain에서 transform을 지원하는 경우(**`capabilities`**의 **`supportedTransforms`** 확인) swap chain에 있는 이미지에 특정 transform(예 시계방향 90도 회전, 수평 플립 등)을 적용할 수 있습니다. 어떤 transform도 지정하고 싶지 않다면 간단하게 current transform을 지정하면 됩니다.

```cpp
createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
```

**`compositeAlpha`** 필드는 Window system에서 다른 윈도우와 블렌딩시 알파 채널을 사용할지를 지정합니다. 여기선 거의 항상 알파 채널을 무시할거라 **`VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR`**를 사용합니다.

```cpp
createInfo.presentMode = presentMode;
createInfo.clipped = VK_TRUE;
```

**`presentMode`** 멤버는 그 이름 그대로입니다. **`clipped`** 멤버가 **`VK_TRUE`**이면 눈에 보이지 않는(예 다른 윈도우에 의해 가려진) 픽셀의 컬러에 대해서는 신경쓰지 않겠다는 의미입니다. 이런한 픽셀을 다시 읽고 예측 가능한 결과를 얻을 필요가 있는게 아니라면 clipping을 활성화해서 최상의 성능을 얻을 수 있습니다.

```cpp
createInfo.oldSwapchain = VK_NULL_HANDLE;
```

마지막 한 필드만 남았습니다. **`oldSwapChain`**입니다. 

Vulkan 사용시 어플리케이션이 실행되는 동안 swap chain이 유효하지 않거나 최적화되지 않을 수 있습니다. 윈도우 리사이즈를 한 예로 들수 있겠습니다. 이 상황에서 swap chain은 실제 처음부터 다시생성해야 합니다. 그리고 이전 swap chain을 참조하기 위해 이 필드에  지정해야 합니다. 이건 복잡한 주제로 앞으로 다룰 [챕터](https://www.notion.so/Drawing-a-triangle-Swap-chain-recreation-c22843c5c73b4f6db0aed3ae74ccc55a)에서 좀 더 알아보도록 하겠습니다. 지금은 swap chain을 하나만 만든다고 가정합니다.

이제 **`CkSwapcahinKHR`** 오브젝트를 저장할 클래스 멤버를 추가합니다.

```cpp
VkSwapchainKHR swapChain;
```

swap chain 생성은 이제 간단히 **`vkCreateSwapchainKHR`**을 호출하면 됩니다.

```cpp
if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
	throw std::runtime_error("failed to create swap chain!");
}
```

파라미터는 논리 디바이스, swap chain 생성 구조체, 선택적 사용자 지정 할당자, 핸들을 저장할 변수의 포인터입니다. 별다른 것은 없습니다. 이 핸들은 논리 디바이스가 파괴되기 전에 **`vkDestroySwapchainKHR`**를 사용하여 정리되어야 합니다.

```cpp
void cleanup() {
    vkDestroySwapchainKHR(device, swapChain, nullptr);
    ...
}
```

이제 어플리케이션을 실행하여 swap chain이 성공적으로 생성되었는지 확인합시다! 이 시점에서 **`vkCreateSwapchainKHR`** 함수에서 access violation이 발생하거나 **`Failed to find 'vkGetInstanceProcAddress' in layer SteamOverlayVulkanLayer.dll`** 같은 메세지를 받는다면 Steam overlay layer에 대한 **[FAQ 항목](https://vulkan-tutorial.com/FAQ)**을 살펴보시기 바랍니다.

Validation layer가 활성화 된 상태에서 **`createInfo.imageExtent = extent;`** 라인을 삭제해 보시기 바랍니다. Validation layer중 하나에서 즉각 실수를 캐치하고 유용한 메세지를 출력하는 걸 볼수 있을 겁니다.



![img](https://dolong.notion.site/image/https%3A%2F%2Fs3-us-west-2.amazonaws.com%2Fsecure.notion-static.com%2Fe8ae3496-b854-46cf-8380-6af2a9bebaf0%2Fswap_chain_validation_layer.png?table=block&id=fd9fe4a1-c9f6-4687-8256-756e7f60402a&spaceId=09757d3e-a434-430f-8919-d37a4c3bb1a4&width=1280&userId=&cache=v2)



## **Retrieving the swap chain images**

이제 swap chain이 생성되었습니다. 이제 남은 것은 swap chain에 있는 **[VkImage](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkImage.html)**들에 대한 핸들을 얻어오는 것입니다. 우리는 다음 챕터에서 렌더링 operation을 진행하는 동안 이들을 참조할 것입니다. 핸들을 저장한 클래스 멤버를 추가합니다.

```cpp
std::vector<VkImage> swapChainImages;
```

이 이미지들은 swap chain을 위한 implementation으로 만들어졌습니다. 그리고 swap chain이 파괴될 때 자동적으로 정리됩니다. 때문에 정리 코드를 추가할 필요가 없습니다.

저는 **`createSwapChain`** 함수의 끝부분에 핸들을 가져오는 코드를 추가할 것입니다. 정확히 **`vkCreateSwapchainKHR`** 호출 뒤에 말이죠. 이 핸들을 가져오는 작업은 다른 시간에 우리가 Vulkan에서 오브젝트 배열을 가져오기 위해 했던 작업과 매우 유사합니다. 

우리는 swap chain에서 최소한의 수만큼만 지정했고 implementation에서 더 많은 수의 swap chain을 생성할 수 있다는 것을 기억하십시오(애초에 필드 이름이 `minImageCount`이니까요). 이것이 우리가 먼저 **`vkGetSwapchainImagesKHR`**를 사용하여 이미지의 최종 갯수를 쿼리한 후 컨텐이너의 크기를 조절하고 마지막으로 이를 다시 호출하여 핸들을 얻어오는 이유입니다.

```cpp
vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
swapChainImages.resize(imageCount);
vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
```

마지막으로, 우리가 swap chain을 생성할 때 swap chain 이미지를 위해 선택했던 format 과 extent을 멤버 변수에 저장합니다. 이 값들을 이후 챕터에서 필요합니다.

```cpp
VkSwapchainKHR swapChain;
std::vector<VkImage> swapChainImages;
VkFormat swapChainImageFormat;
VkExtent2D swapChainExtent;

...

swapChainImageFormat = surfaceFormat.format;
swapChainExtent = extent;
```

이제 우리는 그릴수 있고, 화면에 출력할 수 있는 이미지 세트를 가졌습니다. 

다음 챕터에서는 이미지를 render target으로 설정하는 방법을 배우고 그 다음 실제 graphics pipeline과 drawing commnad를 살펴보기 시작할 것입니다.

**[C++ code](https://vulkan-tutorial.com/code/06_swap_chain_creation.cpp)**





#### ref

https://dolong.notion.site/Drawing-a-triangle-Presentation-Swap-chain-fcd9b3547853434092692928fe1b9cd8
