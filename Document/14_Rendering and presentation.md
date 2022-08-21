## **Setup**

이 챕터는 모든 것이 하나로 모이는 곳입니다. 우리는 main loop에서 호출되어 화면에 삼각형을 올리는 **`drawFrame`** 함수를 작성할 것입니다. 함수를 생성하고 이를 **`mainLoop`**에서 호출하십시오.

```cpp
void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }
}

...

void drawFrame() {

}
```

## **Synchronization**

**`drawFrame`** 함수는 아래의 operation을 수행할 것입니다.

- swap chain으로 부터 image를 획득
- framebuffer에서 해당 image를 attachment로 command buffer를 실행
- presentation을 위해 swap chain에 image를 반환

이런 각 이벤트는 단일 함수 호출로써 동작하도록 설정되지만, 각 이벤트들은 비동기적으로 실행됩니다. 이 함수의 호출은 실제로 operation들이 완료되기 전에 반환되고 실행 순서 또한 정의되어 있지 않습니다. 각 이벤트들은 이전 것들이 완료되는 것에 의존하기 때문에 이런 상황은 유감스럽죠.

swap chain 이벤트들을 동기화 하는 두가지 방법이 있는데, fence 와 semaphore 입니다.  둘 다, 한 operation이 signal을 가지면 다른 operation은 unsignaled에서 signaled 상태로 가기 위해 fence 혹은 semphore를 기다리는 방식으로 operation들을 조정하는데 사용되는 오브젝트입니다.

둘의 차이점은 fence의 상태는 프로그램에서 **[vkWaitForFences](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkWaitForFences.html)** 같은 호출을 통해 접근이 가능하지만 semaphore는 그렇지 않다는 점입니다. Fence는 주로 어플리케이션 자체를 렌더링 operation과 동기화 하기 위해 설계되었고, semaphore는 commnad queue 내에 혹은 command queue 간의 operation을 동기화 하기 위해 사용됩니다. 우리는 draw command와 presentation을 위한 queue operation을 동기화 하려고 하는데 이는 semaphore가 가장 잘 맞습니다.

## **Semaphores**

우리는 image가 획득되었고 그래서 렌더링할 준비가 되었다는 signal을 위해 하나의 semaphore가 필요하고 렌더링이 완료되고 presentation이 발생할 수 있다는 signal을 위해 다른 semaphore가 하나 더 필요합니다. 이 semaphore 오브젝트들을 저장하기 위해서 두개의 클래스 멤버를 생성합니다.

```cpp
VkSemaphore imageAvailableSemaphore;
VkSemaphore renderFinishedSemaphore;
```

semaphore를 생성하기 위해 이 튜토리얼 파트의 마지막 **`create`** 함수인 **`createSemaphores`**를 추가합니다.

```cpp
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffers();
    createSemaphores();
}

...

void createSemaphores() {

}
```

semaphore를 생성하기 위해선 **[VkSemaphoreCreateInfo](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkSemaphoreCreateInfo.html)** 구조체를 채우는게 필요하지만, 현재 버전의 API에서는 **`sType`** 필드 외에는 실제로 요구하는 필드가 없습니다.

```cpp
void createSemaphores() {
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
}
```

이후 버전의 Vulkan API나 extension은 다른 구조체처럼 **`flags`** 나 **`pNext`** 파라미터를 위한 기능이 추가 될 수 있습니다. semaphore의 생성은 **[vkCreateSemaphore](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCreateSemaphore.html)** 를 사용한 친숙한 패턴을 따릅니다.

```cpp
if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {
	throw std::runtime_error("failed to create semaphores!");
}
```

semaphore는 프로그램의 끝에서 모든 command가 완료되고 더 이상 동기화가 필요하지 않을 때 정리가 필요합니다.

```cpp
void cleanup() {
	vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
	vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
```

## **Acquiring an image from the swap chain**

이전에 언급했듯이, **`drawFrame`** 함수에서 첫번째로 해야하는 것은 swap chain에서 image를 얻어오는 것입니다. swap chain은 extension feature라는 것을 상기하십시오. 때문에우리는 **`vk*KHR`** 네이밍 규칙이 있는 함수를 사용해야 합니다.

```cpp
void drawFrame() {
	uint32_t imageIndex;
	vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
}
```

**`vkAcquireNextImageKHR`** 함수의 처음 두개의 파라미터는 우리가 image를 가져오길 원하는 논리 디바이스와 swap chain 입니다. 3번째 파라미터는 나노세컨드 단위의 timeout으로 image가 사용가능해 지기를 기다리는 시간입니다. 64비트 unsigned integer의 최대값을 사용하면 timeout이 비활성화 됩니다.

다음 두 파라미터는 동기화 오브젝트를 지정합니다. presentation 엔진이 image 사용을 완료하면 이 오브젝트에 signal을 보냅니다. 이때가 우리가 image에 drawing을 시작할 수 있는 시점입니다. semaphore나 fence 혹은 둘다를 지정할 수 있습니다. 우리는 이 목적을 위해서 **`imageAvailableSemaphre`**를 사용할 것입니다.

마지막 파라미터는 사용 가능하게 된 swap chain image의 인덱스 값을 저장할 변수입니다. 이 인덱스는 우리의 **`swapChainImages`** 배열에서 **[VkImage](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkImage.html)**를 참조합니다. 우리는 이 인덱스를 사용하여 올바는 command buffer를 선택할 것입니다.

## **Submitting the command buffer**

Queue 제출 및 동기화는 **[VkSubmitInfo](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkSubmitInfo.html)** 구조체의 파라미터를 통해 구성됩니다.

```cpp
VkSubmitInfo submitInfo = {};
submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
submitInfo.waitSemaphoreCount = 1;
submitInfo.pWaitSemaphores = waitSemaphores;
submitInfo.pWaitDstStageMask = waitStages;
```

처음 3개의 파라미터는 어떤 semaphore들이 실행이 시작되기 전에 기다려야 하는지와 어떤 pipeline의 스테이지(들)을 기다려야 하는지를 지정합니다. 우리는 image에 color를 쓰는 작업을 이 image가 가능할 때 까지 기다릴거라, color attachment를 쓰는 graphics pipeline 단계를 지정합니다. 이는 이론적으로 image가 아직 사용가능하지 않은 상태일때 이미 우리의 vertex shader가 시작될 수 있다는 것을 의미합니다. **`waitStages`** 배열의 각 항목은 **`pWaitSemaphores`** 배열의 같은 인덱스에 있는 semaphore와 교신합니다.

```cpp
submitInfo.commandBufferCount = 1;
submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
```

다음 두개의 파라미터는 어떤 command buffer가 실행을 위해서 실제 제출되는지를 지정합니다. 이전에 언급했듯이 방금 획득한 swap chain image를 color attachment로 바인딩하는 command buffer를 제출해야 합니다.

```cpp
VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
submitInfo.signalSemaphoreCount = 1;
submitInfo.pSignalSemaphores = signalSemaphores;
```

**`signalSemaphoreCount`**와 **`pSignalSemaphores`** 파라미터는 command buffer(들)의 실행이 완료되었을 때 signal를 보낼 semaphore(들)을 지정합니다. 우리는 이 목적을 위해 **`renderFinishedSemaphore`** 를 사용했습니다.

```cpp
if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
	throw std::runtime_error("failed to submit draw command buffer!");
}
```

우리는 이제 **[vkQueueSubmit](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkQueueSubmit.html)** 을 사용하여 graphics queue에 command buffer를 제출할 수 있습니다.  이 함수는 작업량이 많을 때 효율적인 처리를 위해서 **[VkSubmitInfo](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkSubmitInfo.html)** 구조체의 배열을 인자로 받습니다. 마지막 파라미터는 선택적 파라미터로 fence를 참조하는데 이는 제출된 모든 command buffer들이 실행을 완료하였을 때 신호를 받습니다.  우리는 동기화를 위해서 semaphore를 사용하기 때문에 이 파라미터는 **`VK_NULL_HANDLE`** 로 패스합니다.

## **Subpass dependencies**

render pass의 subpass가 image layout 전환을 자동적으로 관리한다는 걸 기억하실겁니다. 이 전환은 ***subpass dependencies***에 의해 제어되는데 이는 subpass 사이의 메모리와 실행 종속성을 지정합니다. 우리는 현재 오직 하나의 subpass만 가지고 있지만 이 subpass의 직전/직후의 operation도 암시적인 "subpass"로 카운팅됩니다.

render pass의 시작과 render pass의 종료 시점의 전환을 관리하는 2개의 빌트인 종속성이 있습니다. 하지만 전자(render pass의 시작 시점의 전환을 관리하는 종속성)는 적시에 발생하지 않습니다. 이것은 pipeline의 시작 시점에 전환이 발생할 거라고 가정합니다. 하지만 이 시점에 우리는 아직 image를 획득하지 못한 상태입니다. 이 문제를 다루는데 두가지 방법이 있습니다. 우리는 image가 사용 가능하게 될때 까지 render pass를 시작하지 못하도록 **`imageAvailabeSemaphore`** 을 위한 **`waitStage`** 을 **`VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT`** 로 바꾸거나 render pass를 **`VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT`** 단계를 위해 기다리도록 만들 수 있습니다. 여기선 두번째 방법을 사용할 것입니다. 왜냐면 이 방식이 subpass dependencies와 그들의 동작 방법을 살펴볼수 있는 좋은 구실이기 때문입니다.

subpass dependencies는 **[VkSubpassDependency](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkSubpassDependency.html)** 구조체에 지정됩니다. **`createRenderPass`** 함수로 가서 아래를 추가합니다.

```cpp
VkSubpassDependency dependency = {};
dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
dependency.dstSubpass = 0;
```

처음 두개의 필드는 종속 인덱스와 종속하는 subpass를 지정합니다. 특수값 **`VK_SUBPASS_EXTERNAL`**은 **`srcSubpass`**에 지정되었는지 **`dstSubpass`**에 지정되었는지에 따라 render pass 전/후의 암시적 subpass를 나타냅니다. 인덱스 **`0`**는 우리의 처음이자 하나밖에 없는 subpass를 나타냅니다. 종속성 그래프의 순환을 방지하기 위해서 (subpass 중 하나가 `VK_SUBPASS_EXTERNAL`이 아닌 경우) **`dstSubpass`**의 값은 **`srcSubpass`**의 값보다 커야 합니다.

```cpp
dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
dependency.srcAccessMask = 0;
```

다음 두개의 필드는 대기할 operation과 이 operation이 발생할 단계를 지정합니다. 우리는 우리가 image를 엑세스하기 전에 swap chain이 이 image 읽기 작업을 끝마칠 때까지 기다려야 합니다. 이것은 color attachment output 단계 자체를 기다림으로써 수행할 수 있습니다.

```cpp
dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
```

여기서 기다려야하는 operation은 color attachment 단계에 있으며 color attachment의 읽기/쓰기를 포함합니다.이러한 설정은 실제로 필요하고(그리고 허용될때) 까지 일어나는 전환을 방지합니다 : 우리가 그것에 color를 쓰기 시작할때.

```cpp
renderPassInfo.dependencyCount = 1;
renderPassInfo.pDependencies = &dependency;
```

**[VkRenderPassCreateInfo](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkRenderPassCreateInfo.html)** 구조체는 depencencies 배열 지정을 위해 두개의 필드를 가지고 있습니다.

## **Presentation**

frame를 drawing하는 마지막 단계는 결과를 swap chain에게 다시 제출하여 최종적으로 화면에 표시하는 것입니다. presentation은 **`drawFrame`** 함수의 마지막에서 **`VkPresentInfoKHR`** 구조체를 통해 구성됩니다.

```cpp
VkPresentInfoKHR presentInfo = {};
presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
presentInfo.waitSemaphoreCount = 1;
presentInfo.pWaitSemaphores = signalSemaphores;
```

처음 두 파라미터는 **[VkSubmitInfo](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkSubmitInfo.html)** 에서 처럼 presentation이 발생하기 전까지 기다릴 semaphore를 지정합니다.

```cpp
VkSwapchainKHR swapChains[] = { swapChain };
presentInfo.swapchainCount = 1;
presentInfo.pSwapchains = swapChains;
presentInfo.pImageIndices = &imageIndex;
```

다음 두 파라미터는 image를 표시할 swap chain들과 각 swap chain의 image 인덱스입니다. 이것은 거의 항상 하나입니다.

```cpp
presentInfo.pResults = nullptr; // Optional
```

마지막 선택적 파라미터는 **`pResults`** 입니다. 이것은 presentation이 성공했는지 모든 각각의 swap chain을 체크하기 위한 **[VkResult](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkResult.html)** 값들의 배열을 지정할 수 있게 해줍니다. 단일 swap chain을 사용하는 경우에는 간단하게 present 함수의 리턴 값을 사용하면 되기 때문에 필요하지 않습니다.

```cpp
vkQueuePresentKHR(presentQueue, &presentInfo);
```

**`vkQueuePresentKHR`** 함수는 swap chain에게 image를 표시하라는 요청을 제출합니다. 우리는 다음 챕터에 **`vkAcquireNextImageKHR`**과 **`vkQueuePresentKHR`** 에 에러 처리를 추가할 것입니다. 왜냐면 이 함수들의 실패가 지금까지 봤던 함수들과는 다르게 프로그램을 종료해야할 필요가 있다는 의미는 아니기 때문입니다.

여기까지 모든것을 제대로 했다면 프로그램을 실행했을때 아래와 유사한 무엇인가를 볼 수 있을 겁니다.

![image](https://user-images.githubusercontent.com/16304843/185779711-163a868b-f998-4d1e-a264-c368a38184f3.png)

>이 colored trianlge은 graphics tutorial에서 보던 것과 약간 다르게 보일 수도 있습니다. 이는 이 tutorial이 shader가 linear color space에서 보간하고, 이후 sRGB color space로 변환할 것이기 때문입니다. 이 차이에 대한 논의는 다음 블로그를 참조하시기 바랍니다.

예~~~ !! 허나 불행하게도 validation layer가 활성화 된 상태에서 프로그램을 닫으면 크래시가 발생하는 걸 볼 수 있을 겁니다. debugCallback으로 터미널에 표시되는 메세지는 우리에게 이유를 말해줍니다.

![image](https://user-images.githubusercontent.com/16304843/185779734-9153b4f6-b40d-4b24-a870-e3d3a4e9cf5e.png)

**`drawFrame`**의 모든 operation은 비동기라는걸 기억하십시오. 위 메세지의 의미는 우리가 **`mainLoop`**의 루프를 벗어날 때 drawing과 presentation operation이 아직 진행중이라는 것입니다. 이게 진행되는 동안 리소스를 정리하는건 좋지않은 생각입니다.

이 문제를 해결하기 위해, `mainLoop`를 탈출하고 윈도우를 파괴하기 전에 논리 디바이스가 operation을 끝내는 걸 기다려야 합니다.

```cpp
void mainLoop() {
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		drawFrame();
	}
	vkDeviceWaitIdle(device);
}
```

**[vkQueueWaitIdle](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkQueueWaitIdle.html)** 을 사용하여 특정 command queue의 operation이 끝날때 까지 기다리는 것도 가능합니다. 이 함수들은 동기화를 수행하는 매우 기초적인 방법으로 사용될 수 있습니다. 이제 윈도우를 닫아도 문제 없이 프로그램이 종료되는 것을 볼 수 있을겁니다.

## **Frames in flight**

validation layer를 활성화한 상태에서 어플리케이션을 실행하고 어플리케이션의 메모리 사용율을 모니터링하면 메모리 사용률이 느리게 증가하는 것을 볼 수 있을 겁니다. 이 이유는 어플리케이션이 **`drawFrame`** 함수에서 빠르게 작업을 제출을 하는데 이  작업의 완료를 실제로 체크하고 있지 않기 때문입니다. 만약 CPU의 작업 제출이 GPU가 따라잡을 수 있는 속도보다 빠르면 queue는 작업으로 서서히 채워집니다. 더 나쁜건 우리가 동시에 여러 프레임에서 같은 **`imageAvailableSemaphore`**와 **`renderFinishSemaphore`** 를 재사용하고 있다는 것입니다!

이걸 해결하는 쉬운 방법은 작업 제출 후 그것이 그것이 제대로 끝날때까지 기다리는 것입니다. 예를 들어 **[vkQueueWaitIdle](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkQueueWaitIdle.html)** 을 사용해서 아래와 같이 처리합니다.

```cpp
void drawFrame() {
	...

	vkQueuePresentKHR(presentQueue, &presentInfo);

	vkQueueWaitIdle(presentQueue);
}
```

그러나 이런 방식은 GPU의 최적으로 사용하기에 알맞지 않습니다.  왜냐면 전체 graphics pipeline이 현재 한 프레임에 오직 한번만 사용되기 때문입니다. 현재 프레임이 이미 idel 상태로 진행했고 이 상태는 다음 프레임에서 사용할 수 있습니다.  이제 어플리케이션을 확장하여 쌓여가는 작업량을 제한하면서 여러 프레임이 진행(***in-flight***)될 수 있도록 하겠습니다.

동시에 얼마나 많은 프레임이 진행되어야 하는지를 정의하는 상수값을 프로그램의 최상단에 추가하는 걸로 시작하겠습니다.

```cpp
const int MAX_FRAMES_IN_FLIGHT = 2;
```

각 프레임은 자신만의 semaphore를 가지고 있어야 합니다.

```cpp
std::vector<VkSemaphore> imageAvailableSemaphores;
std::vector<VkSemaphore> renderFinishedSemaphores;
```

이 semaphore들을 생성하도록 `createSemaphores` 함수를 수정합니다.

```cpp
void createSemaphores() {
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create semaphores for a frame!");
		}
	}
}
```

비슷하게 이들을 정리하는 부분도 수정합니다.

```cpp
void cleanup() {
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
	}
	...
}
```

매번 올바른 쌍으로 semaphore들을 사용하기 위해 우리는 현재 프레임을 추적해야 합니다. 이 목적으로 프레임 인덱스를 사용하겠습니다.

```cpp
size_t currentFrame = 0;
```

이제 **`drawFrame`** 함수가 올바른 오브젝트를 사용하도록 수정할 수 있습니다.

```cpp
void drawFrame() {
    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    ...

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};

    ...

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};

    ...
}
```

물론, 매번 다음 프레임을 진행해야 하는 걸 잊으면 안됩니다.

```cpp
void drawFrame() {
	...
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
```

modulo(%) operator를 사용하여 프레임 인덱스가 **`MAX_FRAME_IN_FLIGHT`** 안에서 루핑되도록 합니다.

멀티 프레임 동시 진행을 용이하게 하는 필수적인 오브젝트를 설정했지만, 실제로 **`MAX_FRAMES_IN_FLIGHT`** 을 초과하는 제출을 막지는 않았습니다. 현재 여기에는 GPU-GPU 동기화만 있고 작업 진행상황을 추적하는 CPU-GPU 동기화는 수행되고 있지 않습니다. Frame #0가 진행중인 동안 Frame #0 오브젝트를 사용할 수 있다는 거죠!

CPU-GPU 동기화를 수행하기 위해서, Vulkan은 두번째 유형의 동기화 원형인 ***fence*** 를 제공합니다. fence는 signal를 보내고 기다린다는 측면에서는 semaphore와 유사하지만 , 이번에는 실제로 그들(fence)를 우리 자신의 코드로 기다립니다. 먼저 각 프레임을 위한 fence를 생성합니다.

```cpp
std::vector<VkSemaphore> imageAvailableSemaphores;
std::vector<VkSemaphore> renderFinishedSemaphores;
std::vector<VkFence> inFlightFences;
size_t currentFrame = 0;
```

저는 fence를 semaphore들과 같이 생성하기로 결정했고 그래서 **`createSemaphores`** 함수도 **`createSyncObject`** 라는 이름으로 변경하겠습니다.

```cpp
void createSyncObjects() {
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
						throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}
}
```

fence(**[VkFence](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkFence.html)**)를 생성하는 것은 semaphore를 생성하는 것과 매우 유사합니다. fence를 정리하는 것도 확실하게 합니다.

```cpp
void cleanup() {
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(device, inFlightFences[i], nullptr);
	}
	...
}
```

이제 동기화를 위해 fence를 사용하도록 **`drawFrame`** 함수를 변경합니다. **[vkQueueSubmit](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkQueueSubmit.html)** 호출에는 command buffer가 완료되었을 때 신호를 보내야 하는 fence를 전달하는 선택적 파라미터가 있습니다. 우리는 이를 사용하여 프레임이 완료되었다는 신호를 보낼 수 있습니다.

```cpp
void drawFrame() {
	...
	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}
	...
}
```

이제 남은건 **`drawFrame`** 의 시작에서 프레임이 완료되었다는 것을 기다리도록 함수를 변경하는 것 뿐입니다.

```cpp
void drawFrame() {
	vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
	vkResetFences(device, 1, &inFlightFences[currentFrame]);
	...
}
```

**[vkWaitFences](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkWaitForFences.html)** 함수는 fence의 배열을 받고 리턴하기 전에 하나 또는 모든 펜스가 signaled가 될 때 까지 기다립니다. 여기서 **`VK_TRUE`**가 전달되면 이는 모든 fence를 대기한다는 것을 의미합니다만 우리의 경우 처럼 fence가 하나일 때는 중요하지 않습니다. **`vkAcquireNextImageKHR`**과 마찬가지로 이 함수도 timeout 값을 받습니다. semaphore와는 다르게 **[vkResetFences](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkResetFences.html)** 호출을 사용하여 수동으로 fence를 리셋하여 unsignaled 상태로 복원해야 합니다.

이제 프로그램을 실행시키면 뭔가 이상한점을 발견하게 될것입니다. 어플리케이션이 더 이상 아웃것도 렌더링 하지 않는 것 처럼 보입니다. 문제는 우리가 제출되지 않은 fence를 기다리고 있다는 것입니다. fence는 unsignaled 상태로 생성되고, 이는 **[vkWaitForFence](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkWaitForFences.html)**는 우리가 이 fence 이전에 사용하지 않았다면 영원히 기다릴 것이라는 걸 의미합니다. 이 문제를 해결하기 위해서, 우리가 초기 프레임이 완료된 후에 렌더링 한것 처럼 feace를 생성할때 signaled 상태로 초기화하도록 변경할 수 있습니다.

```cpp
void createSyncObjects() {
	...
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	...
}
```

메모리릭은 사라졌지만, 프로그램은 아직 제대로 작동하지 않습니다. 만약 MAX_FRAMES_IN_FLIGHT가 swap chain image의 수보다 크거나 vkAcquireNextImageKHR이 잘못된 순서의 image를 반환하면 이미 실행 중인 swap chain image로 렌더링을 시작할 수 있습니다. 이를 피하기 위해, 현재 진행중인 프레임에서 swap chain image를 사용하고 있는 경우, 각 swap chain image를 추적해야 합니다. 이 매핑은 그들의 fence에 의해 실행중인 frame을 참조하므로 새 프레임이 해당 이미지를 사용하기 전에 대기할 동기화 오브젝트가 즉시 있습니다.

먼저 이를 추적하기 위한 새로운 `imagesInFlight` 리스트를 추가합니다.

```cpp
std::vector<VkFence> inFlightFences;
std::vector<VkFence> imagesInFlight;
size_t currentFrame = 0;
```

이를 createSyncObjects에서 준비합니다.

```cpp
void createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

    ...
}
```

처음에는 단일 프레임이 image를 사용하지 않으므로, 명시적으로 fence 없음으로 초기화 합니다. 이제 `drawFrame`을 수정하여 새 프레임에 대해 방금 할당한 image를 사용하는 이전 프레임을 기다리도록 합니다.

```cpp
void drawFrame() {
    ...

    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    // Check if a previous frame is using this image (i.e. there is its fence to wait on)
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    // Mark the image as now being in use by this frame
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    ...
}
```

이제 `vkWaitForFence` 호출이 더 많아졌으므로, `vkRestFences` 호출은 이동해야 합니다. fence를 실제로 사용하기 직전에 단순히 호출하는게 가장 좋습니다.

```cpp
void drawFrame() {
    ...

    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    ...
}
```

이제 필요한 모든 동기화를 구현하여 두 프레임 이상의 작업이 큐에 들어가지 않고, 이러한 프레임이 뜻하지 않게 동일한 이미지를 사용하지 않도록 했습니다. 마지막 정리 작업 같은 부분의 동기화 코드는 **[vkDeviceWaitIdle](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkDeviceWaitIdle.html)** 같은 좀 더 개략적인 방법에 의존해도 괜찮습니다. 성능 요구사항을 기준으로 해서 사용할 방법을 결정해야 합니다.

예제를 통해 동기화에 대해서 좀 더 알아보길 원한다면, Khronos에서 제공하는 **[this extensive overview](https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples#swapchain-image-acquire-and-present)**를 참조하십시오.

## **Conclusion**

900라인이 약간 넘은 코드를 지나 우리는 최종적으로 화면에 뭔가를 올리려서 볼 수 있는 단계에 이르렀습니다! Vukan 프로그램을 로드하는 것은 확실이 일이 많습니다. 허나 이것은 Vulkan이 우리에게 그것의 명확성을 통해 엄청난 량의 제어권을 제공한다는 것을 의미합니다. 잠시 시간을 내어 코드를 다시 읽고 프로그램에 있는 모든 Vulkan 오브젝트의 목적과 오브젝트간의 관계를 보여주는 mental model를 작성할 것을 권장합니다. 우리는 이 시점부터 지금 있는 지식위에서 프로그램의 기능을 확장할 것입니다.

다음 챕터에서 Vulkan 프로그램이 잘 동작하기 위해 필요한 한 두가지 작은 부분을 다룰 것입니다.

**[C++ code](https://vulkan-tutorial.com/code/15_hello_triangle.cpp)** / **[Vertex shader](https://vulkan-tutorial.com/code/09_shader_base.vert)** / **[Fragment shader](https://vulkan-tutorial.com/code/09_shader_base.frag)**





