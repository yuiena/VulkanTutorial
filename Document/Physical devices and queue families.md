- physical device 선택
- 기본 장치 적합성 검사
- Queue families

### Queue Family?
 동일한 Property(특성)를 가진 Queue의 집합을 의미

![image](https://user-images.githubusercontent.com/16304843/182026540-aa5cb247-832d-4462-8a95-92c567626f2c.png)

Vulkan에서는 위 Command를 실행하기 위해서 먼저 Command Buffer에 Command를 저장한다. 나중에 Command Buffer를 생성하는 코드 역시 작성해야 한다. 
Command가 저장된 Command Buffer는 다시 Queue를 통해서 GPU로 전달된다

# Physical and Logical Device?
https://www.youtube.com/watch?v=DRl-3c3OJLU
Physical Device는 간단하게 GPU Device를 의미.
Physical Device를 선택하고, Queue Family를 선택하였으면 다음은 Logical Device(SwapChain, 실제 application view)를 생성할 차례이다. 
Logical Device는 Physical Device와 인터페이스(Interface, 통신의 의미로 사용)를 하기 위해서 사용한다. 

---

# physical device 선택

Vulkan 라이브러리를 통해 초기화한 후 [`VkInstance`](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkInstance.html)시스템에서 필요한 기능을 지원하는 그래픽 카드를 찾아 선택해야 합니다. 실제로 그래픽 카드를 원하는 수만큼 선택하여 동시에 사용할 수 있지만 이 자습서에서는 필요에 맞는 첫 번째 그래픽 카드를 사용합니다.

함수 `pickPhysicalDevice`를 추가하고 `initVulkan`함수에 호출을 추가합니다 .

```c++
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    pickPhysicalDevice();
}

void pickPhysicalDevice() {

}
```

최종적으로 선택하게 될 그래픽 카드는 [`VkPhysicalDevice`](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkPhysicalDevice.html)새 클래스 멤버로 추가되는 핸들에 저장됩니다. 

이  [`VkInstance`](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkInstance.html)객체는 소멸될 때 암시적으로 소멸되므로 `cleanup`함수에서 새로운 작업을 수행할 필요가 없습니다 .

```c++
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
```

그래픽 카드를 나열하는 것은 확장을 나열하는 것과 매우 유사하며 deviceCount만 요청하는 것으로 시작합니다.

```c++
uint32_t deviceCount = 0;
vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
```

Vulkan을 지원하는 장치가 0개라면 더 이상 의미가 없습니다.

```c++
if (deviceCount == 0) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
}
```

[`VkPhysicalDevice`](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkPhysicalDevice.html) 그렇지 않으면 이제 모든 핸들 을 보유할 배열을 할당할 수 있습니다 .

```c++
std::vector<VkPhysicalDevice> devices(deviceCount);
vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
```

이제 각 그래픽 카드를 평가하고 모든 그래픽 카드가 동일하게 생성되지 않기 때문에 수행하려는 작업에 적합한지 확인해야 합니다. 이를 위해 우리는 새로운 기능을 도입할 것입니다:

```c++
bool isDeviceSuitable(VkPhysicalDevice device) {
    return true;
}
```

그리고 **Physical Device**가 해당 기능에 추가할 요구 사항을 충족하는지 확인합니다.

```c++
for (const auto& device : devices) {
    if (isDeviceSuitable(device)) {
        physicalDevice = device;
        break;
    }
}

if (physicalDevice == VK_NULL_HANDLE) {
    throw std::runtime_error("failed to find a suitable GPU!");
}
```

다음 섹션에서는 `isDeviceSuitable`함수에서 확인할 첫 번째 요구 사항을 소개합니다 . 

이후 장에서 더 많은 Vulkan 기능을 사용하기 시작할 것이므로 더 많은 검사를 포함하도록 이 기능을 확장할 것입니다.



# 기본 장치 적합성 검사

장치의 적합성을 평가하기 위해 먼저 몇 가지 세부 사항을 요청할 수 있습니다. 이름, 유형 및 지원되는 Vulkan 버전과 같은 기본 장치 속성을 [`vkGetPhysicalDeviceProperties`](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkGetPhysicalDeviceProperties.html)를 사용하여 요청할 수 있습니다 .

```c++
VkPhysicalDeviceProperties deviceProperties;
vkGetPhysicalDeviceProperties(device, &deviceProperties);
```

Texture Compression, 64bit float 및 multi viewport rendering(VR에 유용)과 같은 선택적 기능에 대한 지원은 다음 [`vkGetPhysicalDeviceFeatures`](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkGetPhysicalDeviceFeatures.html)을 사용하여 요청할 수 있습니다 .

```c++
VkPhysicalDeviceFeatures deviceFeatures;
vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
```

device memory 및 **queue families**와 관련하여 나중에 요청할 device에서 요청할 수 있는 자세한 내용이 있습니다(다음 섹션 참조).

예를 들어 geometry shader를 지원하는 전용 그래픽 카드에만 사용할 수 있는 애플리케이션을 고려해보자. 

그러면 `isDeviceSuitable` 함수는 다음과 같이 보일 것입니다.

```c++
bool isDeviceSuitable(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
           deviceFeatures.geometryShader;
}
```

device가 적합한지 여부를 확인하고 첫 번째 device를 사용하는 대신 각 device에 점수를 부여하고 가장 높은 device를 선택할 수도 있습니다. 

그렇게 하면 더 높은 점수를 부여하여 전용 그래픽 카드를 선호할 수 있지만 사용 가능한 유일한 GPU인 경우 통합 GPU로 대체할 수 있습니다. 

다음과 같이 구현할 수 있습니다.

```c++
#include <map>

...

void pickPhysicalDevice() {
    ...

    // Use an ordered map to automatically sort candidates by increasing score
    std::multimap<int, VkPhysicalDevice> candidates;

    for (const auto& device : devices) {
        int score = rateDeviceSuitability(device);
        candidates.insert(std::make_pair(score, device));
    }

    // Check if the best candidate is suitable at all
    if (candidates.rbegin()->first > 0) {
        physicalDevice = candidates.rbegin()->second;
    } else {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

int rateDeviceSuitability(VkPhysicalDevice device) {
    ...

    int score = 0;

    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    // Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;

    // Application can't function without geometry shaders
    if (!deviceFeatures.geometryShader) {
        return 0;
    }

    return score;
}
```

이 튜토리얼에서 모든 것을 구현할 필요는 없지만 device 선택 프로세스를 설계할 수 있는 방법에 대한 아이디어를 제공하기 위한 것입니다. 물론 선택 항목의 이름을 표시하고 사용자가 선택하도록 할 수도 있습니다.

우리는 이제 막 시작하기 때문에 Vulkan 지원이 우리에게 필요한 유일한 것이므로 모든 GPU로 정착할 것입니다.

```c++
bool isDeviceSuitable(VkPhysicalDevice device) {
    return true;
}
```

다음 섹션에서는 확인해야 할 첫 번째 실제 필수 기능에 대해 설명합니다.



# Queue families

그리기에서 텍스처 업로드에 이르기까지 Vulkan의 거의 모든 작업을 수행하려면 queue에 command을 submit해야 한다는 점에 대해 간단히 설명했습니다. 

서로 다른 *Queue Type* 에서 시작되는 서로 다른 유형의 queue가 있으며 각 queue 계열은 command의 하위 집합만 허용합니다. 예를 들어, 컴퓨팅 명령의 처리만 허용하는 **queue family** 제품군이나 메모리 전송 관련 명령만 허용하는 대기열 제품군이 있을 수 있습니다.

device가 지원하는 **queue family**와 사용하려는 command을 지원하는 **queue family**를 확인해야 합니다. `findQueueFamilies`이를 위해 우리 는 필요한 모든 **queue family**를 찾는 새로운 기능을 추가할 것입니다.

![image](https://user-images.githubusercontent.com/16304843/182026577-1865b7c6-28a4-491f-9f53-085882b8c2f3.png)

지금은 그래픽 명령을 지원하는 queue만 찾을 것이므로 함수는 다음과 같을 수 있습니다.

```c++
uint32_t findQueueFamilies(VkPhysicalDevice device) {
    // Logic to find graphics queue family
}
```

그러나 다음 장 중 하나에서 이미 또 다른 queue을 찾을 것이므로 이에 대비하고 인덱스를 구조체로 묶는 것이 좋습니다.

```c++
struct QueueFamilyIndices {
    uint32_t graphicsFamily;
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;
    // Logic to find queue family indices to populate struct with
    return indices;
}
```

그러나 **queue family**를 사용할 수 없는 경우에는 어떻게 합니까? 에서 예외를 throw할 수 `findQueueFamilies`있지만 이 함수는 실제로 device 적합성에 대한 결정을 내리는 데 적합한 위치가 아닙니다. 

예를 들어, 전용 전송 대기열 제품군이 있는 device를 *선호* 할 수 있지만 필요하지는 않습니다. 따라서 특정 **queue family**가 발견되었는지 여부를 나타내는 방법이 필요합니다.

`uint32_t`이론적으로 의 값은 `0`. 운 좋게도 C++17에는 값이 존재하는지 여부를 구분하는 데이터 구조가 도입되었습니다.

```c++
#include <optional>

...

std::optional<uint32_t> graphicsFamily;

std::cout << std::boolalpha << graphicsFamily.has_value() << std::endl; // false

graphicsFamily = 0;

std::cout << std::boolalpha << graphicsFamily.has_value() << std::endl; // true
```

`std::optional`무언가를 할당할 때까지 값을 포함하지 않는 래퍼입니다. `has_value()`언제든지 멤버 함수 를 호출하여 값이 포함되어 있는지 여부를 요청할 수 있습니다 . 즉, 논리를 다음과 같이 변경할 수 있습니다.

```
#include <optional>

...

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;
    // Assign index to queue families that could be found
    return indices;
}
```

이제 실제로 `findQueueFamilies`구현을 시작할 수 있습니다 .

```c++
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    ...

    return indices;
}
```

**queue family** 목록을 검색하는 프로세스는 정확히 예상하고 사용하는 것입니다 [`vkGetPhysicalDeviceQueueFamilyProperties`](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkGetPhysicalDeviceQueueFamilyProperties.html).

```c++
uint32_t queueFamilyCount = 0;
vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
```

[`VkQueueFamilyProperties`](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkQueueFamilyProperties.html)구조체에는 지원되는 작업 유형 및 해당 제품군을 기반으로 생성할 수 있는 queue 수를 포함하여 queue  제품군에 대한 몇 가지 세부 정보가 포함되어 있습니다 . `VK_QUEUE_GRAPHICS_BIT`를 지원하는 **queue family**를 하나 이상 찾아야 합니다 .

```c++
int i = 0;
for (const auto& queueFamily : queueFamilies) {
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        indices.graphicsFamily = i;
    }

    i++;
}
```

이제 이 멋진 **queue family** 조회 기능이 있으므로 이 `isDeviceSuitable`기능을 사용하여 사용하려는 command을 device가 처리할 수 있는지 확인하는 기능의 검사로 사용할 수 있습니다.

```c++
bool isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);

    return indices.graphicsFamily.has_value();
}
```

이것을 좀 더 편리하게 만들기 위해 구조체 자체에 일반 검사도 추가합니다.

```c++
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;

    bool isComplete() {
        return graphicsFamily.has_value();
    }
};

...

bool isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);

    return indices.isComplete();
}
```

이제 다음에서 조기 종료를 위해 이것을 사용할 수도 있습니다 `findQueueFamilies`.

```c++
for (const auto& queueFamily : queueFamilies) {
    ...

    if (indices.isComplete()) {
        break;
    }

    i++;
}
```

좋습니다. 올바른 physical device를 찾는 데 필요한 것은 이것뿐입니다! 다음 단계는 그것과 인터페이스할 [logical device를 만드는 것입니다.](https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Logical_device_and_queues)

[C++ 코드](https://vulkan-tutorial.com/code/03_physical_device_selection.cpp)







