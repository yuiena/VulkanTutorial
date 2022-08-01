# Validation layers란?

Vulkan API는 드라이버 오버헤드를 최소화한다는 아이디어를 중심으로 설계되었으며 그 목표의 표현 중 하나는 `기본적으로 API에서 매우 제한된 error check가 있다는 것`입니다.

잘못된 값으로 enum을 설정하거나 필수 매개변수에 null 포인터를 전달하는 것과 같은 단순한 실수도 일반적으로 명시적으로 처리되지 않으며 단순히 충돌이나 정의되지 않은 동작을 초래합니다.

Vulkan운 수행하는 모든 작업에 대해 매우 명시적이야 하기 때문에 새로운 GPU 기능을 사용하거나 logical device 생성 시 요청하는 것을 잊어버리는 등의 작은 실수를 하기 쉽습니다.

그러나 이것이 이러한 검사를 API에 추가할 수 없다는 것을 의미하지는 않습니다.

Vulkan은 **Validation Layer**로 알려진 이를 위한 우아한 시스템을 도입했습니다. **Validation Layer**는 Vulkan 함수 호출에 연결하여 추가 작업을 적용하는 선택적 구성 요소 입니다.

**Validation Layer**의 일반적인 작업은 다음과 같습니다.



- 잘못된 사용을 감지하기 위해 사양에 대한 매개변수 값 확인 
- 리소스 leak을 찾기 위해 개체 생성 및 파괴 추적
- 호출이 시작된 스레드를 추적하여 스레드 안전성 확인
- 모든 호출과 매개변수를 표준 출력에 기록
- 프로파일링 및 재생을 위한 Vulkan 호출 추적



다음은 **Validation Layer**에서 기능을 구현하는 방법의 예 입니다.



```c++
VkResult vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* instance) {

    if (pCreateInfo == nullptr || instance == nullptr) {
        log("Null pointer passed to required parameter!");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    return real_vkCreateInstance(pCreateInfo, pAllocator, instance);
}
```



이러한 유효성 검사 계층은 관심 있는 모든 디버깅 기능을 포함하도록 자유롭게 쌓을 수 있습니다. 디버그 빌드에 대해 유효성 검사 계층을 활성화하고 릴리스 빌드에 대해 완전히 비활성화하면 두 가지 장점을 모두 누릴 수 있습니다!

Vulkan에는 검증 레이어가 내장되어 있지 않지만 LunarG Vulkan SDK는 일반적인 오류를 확인하는 훌륭한 레이어 세트를 제공합니다. 또한 완전히 [오픈 소스](https://github.com/KhronosGroup/Vulkan-ValidationLayers) 이므로 그들이 어떤 종류의 실수를 확인하고 기여하는지 확인할 수 있습니다. 유효성 검사 계층을 사용하는 것은 정의되지 않은 동작에 실수로 의존하여 애플리케이션이 다른 드라이버에서 중단되는 것을 방지하는 가장 좋은 방법입니다.

검증 계층은 시스템에 설치된 경우에만 사용할 수 있습니다. 예를 들어 LunarG 유효성 검사 레이어는 Vulkan SDK가 설치된 PC에서만 사용할 수 있습니다.

Vulkan에는 이전에 두 가지 유형의 유효성 검사 계층이 있었습니다. 인스턴스 및 장치별입니다. 아이디어는 인스턴스 레이어가 인스턴스와 같은 전역 Vulkan 객체와 관련된 호출만 확인하고 장치별 레이어는 특정 GPU와 관련된 호출만 확인한다는 것이었습니다. 기기별 레이어는 이제 더 이상 사용되지 않습니다. 즉, 인스턴스 유효성 검사 레이어가 모든 Vulkan 호출에 적용됩니다. 사양 문서에서는 일부 구현에 필요한 호환성을 위해 장치 수준에서 유효성 검사 계층을 활성화할 것을 여전히 권장합니다. [논리적 장치 수준에서 인스턴스와 동일한 계층을 지정하기만 하면 됩니다. 이에 대해서는 나중에](https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Logical_device_and_queues) 살펴보겠습니다 .



# Validation Layer 사용

이 섹션에서는 Vulkan SDK에서 제공하는 표준 진단 계층을 활성화하는 방법을 살펴보겠습니다. 

확장과 마찬가지로  **Validation Layer**는 이름을 지정하여 활성화해야 합니다. 모든 유용한 표준 유효성 검사는 로 알려진 SDK에 포함된 Layer에 번들로 제공됩니다 `VK_LAYER_KHRONOS_validation`.

먼저 활성화할 계층과 활성화 여부를 지정하기 위해 프로그램에 두 개의 구성 변수를 추가해 보겠습니다. 

프로그램이 디버그 모드에서 컴파일되는지 여부에 따라 해당 값을 기반으로 하기로 선택했습니다. 매크로는 C++ 표준 의 `NDEBUG`일부이며 "디버그 아님"을 의미합니다.

```c++
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif
```

`checkValidationLayerSupport`요청된 모든 레이어를 사용할 수 있는지 확인 하는 새 기능을 추가합니다 . 

먼저 [`vkEnumerateInstanceLayerProperties`](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkEnumerateInstanceLayerProperties.html)함수를 사용하여 사용 가능한 모든 레이어를 나열합니다. 

사용법은 [`vkEnumerateInstanceExtensionProperties`](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkEnumerateInstanceExtensionProperties.html)인스턴스 생성 장에서 설명한 것과 동일합니다.

```c++
for (const char* layerName : validationLayers) 
{
    bool layerFound = false;

    for (const auto& layerProperties : availableLayers) 
    {
        if (strcmp(layerName, layerProperties.layerName) == 0) 
        {
            layerFound = true;
            break;
        }
    }

    if (!layerFound) {
        return false;
    }
}

return true;
```

이제 다음에서 이 기능을 사용할 수 있습니다 `createInstance`.

```c++
void createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }
    ...
}
```

이제 디버그 모드에서 프로그램을 실행하고 오류가 발생하지 않는지 확인하십시오. 그렇다면 FAQ를 살펴보십시오.

마지막으로 [`VkInstanceCreateInfo`](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkInstanceCreateInfo.html)활성화된 경우 유효성 검사 레이어 이름을 포함하도록 구조체 인스턴스화를 수정합니다.

```c++
if (enableValidationLayers) 
{
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
} else 
{
    createInfo.enabledLayerCount = 0;
}
```

검사가 성공 [`vkCreateInstance`](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCreateInstance.html)하면 오류가 반환되지 않아야 `VK_ERROR_LAYER_NOT_PRESENT`하지만 프로그램을 실행하여 확인해야 합니다.





# Message Callback

**Validation Layer**은 기본적으로 디버그 메시지를 표준 출력으로 인쇄하지만 프로그램에서 명시적 콜백을 제공하여 직접 처리할 수도 있습니다. 

또한 모든 메시지가 반드시 (치명적인) 오류가 아니기 때문에 보고 싶은 메시지 종류를 결정할 수 있습니다. 지금 당장 하고 싶지 않다면 이 장의 마지막 섹션으로 건너뛸 수 있습니다.

메시지 및 관련 세부 정보를 처리하기 위해 프로그램에서 콜백을 설정하려면 `VK_EXT_debug_utils`확장을 사용하여 콜백이 있는 디버그 메신저를 설정해야 합니다.

`getRequiredExtensions`먼저 유효성 검사 계층이 활성화되었는지 여부에 따라 필요한 확장 목록을 반환 하는 함수를 만듭니다 .



```c++
std::vector<const char*> getRequiredExtensions() 
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}
```

GLFW에서 지정한 확장은 항상 필요하지만 디버그 메신저 확장은 조건부로 추가됩니다.

 `VK_EXT_DEBUG_UTILS_EXTENSION_NAME`여기서 리터럴 문자열 "VK_EXT_debug_utils"와 동일한 매크로를 사용했습니다 . 이 매크로를 사용하면 오타를 피할 수 있습니다.

이제 다음에서 이 기능을 사용할 수 있습니다 `createInstance `:

```c++
auto extensions = getRequiredExtensions();
createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
createInfo.ppEnabledExtensionNames = extensions.data();
```

`VK_ERROR_EXTENSION_NOT_PRESENT`오류가 발생 하지 않도록 프로그램을 실행하십시오 . 유효성 검사 계층의 가용성에 의해 암시되어야 하기 때문에 이 확장의 존재를 확인할 필요가 없습니다.

이제 디버그 콜백 함수가 어떻게 생겼는지 봅시다. 프로토타입 `debugCallback`과 함께 호출되는 새 정적 멤버 함수 `PFN_vkDebugUtilsMessengerCallbackEXT`를 추가합니다 . 

함수 에 Vulkan이 호출할 수 있는 올바른 서명이 있는지 확인합니다. `VKAPI_ATTR` and `VKAPI_CALL`

```c++
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}
```

첫 번째 매개변수는 다음 플래그 중 하나인 메시지의 심각도를 지정합니다.

- `VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT`: 진단 메시지
- `VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT`: 리소스 생성과 같은 정보 메시지
- `VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT`: 반드시 오류는 아니지만 애플리케이션의 버그일 가능성이 매우 높은 동작에 대한 메시지
- `VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT`: 유효하지 않고 충돌을 일으킬 수 있는 동작에 대한 메시지

이 열거형의 값은 비교 작업을 사용하여 메시지가 특정 심각도 수준과 같거나 더 나쁜지 확인할 수 있는 방식으로 설정됩니다. 예를 들면 다음과 같습니다.

```c++
if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    // Message is important enough to show
}
```

`messageType`매개변수는 다음 값을 가질 수 있습니다 .



- `VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT`: 사양이나 성능과 무관한 이벤트가 발생한 경우
- `VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT`: 사양을 위반하거나 가능한 실수를 나타내는 일이 발생했습니다.
- `VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT`: Vulkan의 잠재적인 최적화 되지 않은 사용



`pCallbackData`매개변수는 메시지 자체의 세부사항을 포함하는 구조체 `VkDebugUtilsMessengerCallbackDataEXT`를 참조하며, 가장 중요한 구성원은 다음과 같습니다.



- `pMessage`: null로 끝나는 문자열로서의 디버그 메시지

- `pObjects`: 메시지와 관련된 Vulkan 객체 핸들의 배열

- `objectCount`: 배열의 객체 수

  

마지막으로 `pUserData`매개변수에는 콜백을 설정하는 동안 지정된 포인터가 포함되어 있으며 이를 통해 고유한 데이터를 전달할 수 있습니다.

콜백은 유효성 검사 계층 메시지를 트리거한 Vulkan 호출을 중단해야 하는지 여부를 나타내는 부울을 반환합니다. 콜백이 true를 반환하면 `VK_ERROR_VALIDATION_FAILED_EXT`오류와 함께 호출이 중단됩니다. 이것은 일반적으로 유효성 검사 레이어 자체를 테스트하는 데만 사용되므로 항상 `VK_FALSE`를 반환해야 합니다 .

이제 남은 것은 콜백 함수에 대해 Vulkan에 알리는 것뿐입니다. 아마도 다소 놀랍게도 Vulkan의 디버그 콜백조차도 명시적으로 생성 및 소멸되어야 하는 핸들로 관리됩니다. 이러한 콜백은 *디버그 메신저* 의 일부이며 원하는 만큼 가질 수 있습니다. 바로 아래에 이 핸들에 대한 클래스 멤버를 추가합니다 `instance`.

```c++
VkDebugUtilsMessengerEXT debugMessenger;
```

이제 바로 `initVulkan`다음 `createInstance`에 호출 함수  `setupDebugMessenger`를 추가합니다 .

```c++
void initVulkan() {
    createInstance();
    setupDebugMessenger();
}

void setupDebugMessenger() {
    if (!enableValidationLayers) return;

}
```


메신저와 콜백에 대한 세부 정보로 구조를 채워야 합니다.

```c++
VkDebugUtilsMessengerCreateInfoEXT createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
createInfo.pfnUserCallback = debugCallback;
createInfo.pUserData = nullptr; // Optional
```

이 `messageSeverity`필드를 사용하면 콜백을 호출하려는 모든 유형의 심각도를 지정할 수 있습니다.

 `VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT`장황한 일반 디버그 정보를 생략하면서 가능한 문제에 대한 알림을 수신하기 위해 여기를 제외한 모든 유형을 지정 했습니다.

마찬가지로 `messageType`필드를 사용하면 콜백에 알림을 받는 메시지 유형을 필터링할 수 있습니다. 여기에서 모든 유형을 단순히 활성화했습니다. 유용하지 않은 경우 언제든지 비활성화할 수 있습니다.

마지막으로 `pfnUserCallback`필드는 콜백 함수에 대한 포인터를 지정합니다. `pUserData`매개변수 를 통해 콜백 함수에 전달될 필드에 대한 포인터를 선택적으로 전달할 수 있습니다 `pUserData`. 예를 들어 이것을 사용하여 `HelloTriangleApplication`클래스에 대한 포인터를 전달할 수 있습니다.

유효성 검사 계층 메시지를 구성하고 콜백을 디버그하는 방법은 더 많지만 이 자습서를 시작하기에 좋은 설정입니다. 

가능성에 대한 자세한 내용은 [확장 사양 을 참조하세요.](https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/chap50.html#VK_EXT_debug_utils)

이 구조체는 객체 `vkCreateDebugUtilsMessengerEXT`를 생성하기 위해 함수에 전달되어야 합니다. `VkDebugUtilsMessengerEXT`안타깝게도 이 함수는 확장 함수이기 때문에 자동으로 로드되지 않습니다. 를 사용하여 주소를 직접 찾아야 합니다 [`vkGetInstanceProcAddr`](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkGetInstanceProcAddr.html). 이를 백그라운드에서 처리하는 자체 프록시 함수를 만들 것입니다. `HelloTriangleApplication`클래스 정의 바로 위에 추가했습니다 .

```c++
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}
```

함수를 로드할 수 없으면 [`vkGetInstanceProcAddr`](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkGetInstanceProcAddr.html)함수가 `nullptr`을 반환 합니다 . 

이제 이 함수를 호출하여 사용 가능한 경우 확장 개체를 만들 수 있습니다.

```c++
if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
    throw std::runtime_error("failed to set up debug messenger!");
}
```

`nullptr`마지막에서 두 번째 매개변수는 매개변수가 매우 간단하다는 점을 제외하고는 우리가 설정한 선택적 할당자 콜백입니다 . 

DEBUG Messanger는 Vulkan Instance와 해당 계층에 고유하므로 첫 번째 인수로 명시적으로 지정해야 합니다. 나중에 다른 *Child Object* 에서도 이 패턴을 볼 수 있습니다 .

`vkDestroyDebugUtilsMessengerEXT`에 대한 `VkDebugUtilsMessengerEXT`호출로 개체도 정리해야 합니다 . 함수와 유사하게 `vkCreateDebugUtilsMessengerEXT` 명시적으로 로드해야 합니다.

바로 아래에 다른 프록시 기능을 만듭니다 `CreateDebugUtilsMessengerEXT`.

```c++
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}
```





# Instance Create 및 destruction(소멸) 디버깅



이제 유효성 검사 레이어를 사용한 디버깅을 프로그램에 추가했지만 아직 모든 것을 다루지는 않습니다. `vkCreateDebugUtilsMessengerEXT`호출하려면 유효한 인스턴스가 생성 `vkDestroyDebugUtilsMessengerEXT`되어야 하며 인스턴스가 소멸되기 전에 호출되어야 합니다 . 이로 인해 현재 [`vkCreateInstance`](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCreateInstance.html)및 [`vkDestroyInstance`](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkDestroyInstance.html)호출에서 문제를 디버깅할 수 없습니다.

그러나 [확장 문서](https://github.com/KhronosGroup/Vulkan-Docs/blob/master/appendices/VK_EXT_debug_utils.txt#L120) 를 자세히 읽어 보면 이 두 함수 호출을 위해 특별히 별도의 디버그 유틸리티 메신저를 만드는 방법이 있음을 알 수 있습니다. 의 확장 필드에 있는 `VkDebugUtilsMessengerCreateInfoEXT`구조체에 대한 포인터를 전달하기만 하면 됩니다 . 메신저의 첫 번째 추출 인구는 정보를 별도의 기능으로 만듭니다.`pNext`[`VkInstanceCreateInfo`](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkInstanceCreateInfo.html)



```
void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

...

void setupDebugMessenger() {
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}
```

`createInstance`이제 함수 에서 이것을 재사용할 수 있습니다 .

```
void createInstance() {
    ...

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    ...

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

```

변수 는 호출 `debugCreateInfo`전에 소멸되지 않도록 if 문 외부에 배치됩니다 . [`vkCreateInstance`](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCreateInstance.html)이 방법으로 추가 디버그 메신저를 생성하면 자동으로 사용되며 [`vkCreateInstance`](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCreateInstance.html)그 [`vkDestroyInstance`](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkDestroyInstance.html)후에 정리됩니다.



# Test

이제 유효성 검사 레이어가 작동하는 것을 보기 위해 의도적으로 실수를 해보자. 함수 `DestroyDebugUtilsMessengerEXT`에서 에 대한 호출을 일시적으로 제거하고 프로그램을 실행합니다. `cleanup`종료되면 다음과 같이 표시되어야 합니다.



| 메시지가 표시되지 않으면 [설치를 확인하십시오](https://vulkan.lunarg.com/doc/view/1.2.131.1/windows/getting_started.html#user-content-verify-the-installation) .

메시지를 트리거한 호출을 확인하려면 메시지 콜백에 중단점을 추가하고 스택 추적을 볼 수 있습니다.





# 구성

`VkDebugUtilsMessengerCreateInfoEXT`구조체 에 지정된 플래그보다 유효성 검사 레이어의 동작에 대한 설정이 훨씬 더 많습니다 . Vulkan SDK로 이동하여 `Config`디렉터리로 이동합니다. `vk_layer_settings.txt`거기 에서 레이어를 구성하는 방법을 설명하는 파일을 찾을 수 있습니다 .

자신의 응용 프로그램에 대한 계층 설정을 구성하려면 파일을 프로젝트의 `Debug`및 `Release`디렉터리에 복사하고 지침에 따라 원하는 동작을 설정합니다. 그러나 이 자습서의 나머지 부분에서는 기본 설정을 사용한다고 가정합니다.

이 튜토리얼 전체에서 유효성 검사 레이어가 이러한 레이어를 잡는 데 얼마나 도움이 되는지 보여주고 Vulkan으로 무엇을 하고 있는지 정확히 아는 것이 얼마나 중요한지 알려주기 위해 몇 가지 의도적인 실수를 범할 것입니다. [이제 시스템에서 Vulkan 장치](https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Physical_devices_and_queue_families) 를 살펴볼 차례 입니다.

[C++ 코드](https://vulkan-tutorial.com/code/02_validation_layers.cpp)
