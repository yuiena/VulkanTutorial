Vulkan의 command들은 drawing operation이나 memory 전송 처럼 함수 호출을 통해 직접적으로 수행되지 않습니다. 수행하고자하는 모든 operation을 commnad buffer 오브젝트에 기록해야 합니다. 이 방식의 장점은 drawing command를 위한 모든 어려운 설정 작업들이 사전에 그리고 여러 스레드에서 수행할 수 있다는 점입니다. 그 이후에 프로그램의 main loop에서 Vulkan에게 command들을 실행해 달라고 말하면 됩니다.

## **Command pools**

command buffer들을 만들기 전에 command pool을 만들어야 합니다. command pool은 버퍼를 저장하는데 사용되는 메모리를 관리하며 command buffer는 그 메모리로 부터 할당됩니다. **[VkCommandPool](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkCommandPool.html)**을 저장할 새로운 클래스 멤버를 추가합니다.

```cpp
VkCommandPool commandPool;
```

그리고 새로운 **`createCommandPool`** 함수를 생성하고 **`initVulkan`** 에서 framebuffer들 생성 후에 호출합니다.

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
}

...

void createCommandPool() {

}
```

command pool 생성에는 오직 두개의 파라미터만 필요합니다.

```cpp
QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

VkCommandPoolCreateInfo poolInfo{};
poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
poolInfo.flags = 0; // Optional
```

command buffer들은 우리가 얻어온 graphics 나 presentation queue같은 디바이스 queue 중 하나에 그들을 제출함으로써 실행됩니다. 각 command pool은 단일 유형의 queue에 제출된 command buffer만 할당할 수 있습니다. 우리는 drawing을 위해서 command들을 기록할거라 graphics queue family를 선택했습니다.

command pool에는 두가지 flag가 가능합니다.

- **`VK_COMMAND_POOL_CREATE_TRANSIENT_BIT`** : command buffer에 새로운 command가 매우 자주(메모리 할당 동작이 변경 될 수 있음) 기록된다는 Hint를 줍니다.
- **`VK_COMMNAD_POOL_CREATE_RESET_COMMAND_BUFFER_BIT`** : command buffer 가 개별적으로 재기록될 수 있도록 해줍니다. 이 플래그가 없으면 모두 같이 재설정되어야 합니다.

우리는 프로그램 시작시에만 comman buffer를 기록하고 main loop 상에서 여러번 실행할거라 어떤 플래그도 사용하지 않겠습니다.

```cpp
if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
	throw std::runtime_error("failed to create command pool!");
}
```

**[vkCreateCommandPool](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCreateCommandPool.html)** 함수를 사용하여 command pool 생성을 완료합니다. 여기에는 별다른 특별한 파라미터가 없습니다. command들은 이 프로그램 전반에서 화면에 그리는 것을 위해 사용되기 때문에 프로그램의 끝에서만 그들을 폐기해야 합니다.

```cpp
void cleanup() {
	vkDestroyCommandPool(device, commandPool, nullptr);
	...
}
```

## **Command buffer allocation**

이제 우리는 command buffer를 할당하고 거기에 drawing commnad를 기록할 수 있습니다. drawing command 중 하나가 올바를 **[VkFramebuffer](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkFramebuffer.html)** 의 바인딩을 수반하기 때문에 실제로 swap chain의 모든 이미지를 위해서 command buffer를 다시 기록해야 합니다. 이를 위해 **[VkCommandBuffer](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkCommandBuffer.html)** 오브젝트의 리스트를 클래스 멤버로 생성합니다. commnad buffer는 command pool이 폐기될 때 자동적으로 해제되기 때문에 명시적인 정리작업은 필요없습니다.

```cpp
std::vector<VkCommandBuffer> commandBuffers;
```

우리는 이제 **`createCommnadBuffers`** 함수에서 각 swap chain image들을 위한 command를 할당/기록할 것입니다.

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
}

...

void createCommandBuffers() {
    commandBuffers.resize(swapChainFramebuffers.size());
}
```

command buffer는 **[vkAllocateCommandBuffers](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkAllocateCommandBuffers.html)** 함수로 할당되고, 이 함수는 command pool과 할당할 buffer의 수가 지정된 **[VkCommandBufferAllocateInfo](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkCommandBufferAllocateInfo.html)** 구조체를 파라미터로 사용합니다.

```cpp
VkCommandBufferAllocateInfo allocInfo = {};
allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
allocInfo.commandPool = commandPool;
allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
	throw std::runtime_error("failed to allocate command buffers!");
}
```

**`level`** 파라미터는 할당되는 command buffer들이 primary 혹은 secondary command buffer인지 지정합니다.

- **`VK_COMMAND_BUFFER_LEVEL_PRIMARY`** : 실행을 위해 queue에 제출될 수 있지만 다른 command buffer에서 호출 될 수 없습니다.
- **`VK_COMMAND_BUFFER_LEVEL_SECONDARY`** : 직접 실행시킬수는 없지만 primary command buffer에서 호출할 수 있습니다.

우리는 여기서 secondary command buffer의 기능을 사용하지 않을 것이지만 이것이 primary command buffer에서 공통적인 operation을 재사용하는데 도움이 될것이라고 생각해 볼 수 있습니다.

## **Starting command buffer recording**

우리는 **[vkBeginCommandBuffer](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkBeginCommandBuffer.html)**을 **[VkCommandBufferBeginInfo](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkCommandBufferBeginInfo.html)**라는 작은 구조체를 인자로 사용해 호출함으로써 command buffer를 기록하기 시작합니다. 이 구조체는 특정 command buffer의 사용법에 대한 특정 세부정보를 지정하고 있습니다.

```cpp
for (size_t i = 0; i < commandBuffers.size(); i++) {
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // operional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}
}
```

**`flags`** 파라미터는 command buffer를 사용하는 방법을 지정합니다. 다음의 값들이 가능합니다.

- **`VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT`** : command buffer가 그것을 실행한 후 바로 재기록 될 수 잇습니다.
- **`VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT`** : 이것은 단일 render pass에 완전히 포함되는 second command buffer 입니다.
- **`VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT`** : command buffer는 그것이 이미 실행을 대기 중일 지라도 다시 제출될 수 있습니다.

이 플래그 중 어느 것도 현재 적용할 수 없습니다.

**`pInheritanceInfo`** 파라미터는 오직 secondary command buffer에만 관계된 파라미터 입니다. 이는 호출하는 primary command buffer로 부터 상속받을 상태를 지정합니다.

command buffer가 이미 한번 기록되었다면, 그 후 **[vkBeginCommandBuffer](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkBeginCommandBuffer.html)** 호출은 암시적으로 이 command buffer를 리셋합니다. 나중에 command buffer에 새로운 command 를 추가하는 것은 불가능합니다.

## **Starting a render pass**

**[vkCmdBeginRenderPass](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdBeginRenderPass.html)**로 render pass를 시작해서 drawing을 시작합니다. render pass는 **[VkRenderPassBeginInfo](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkRenderPassBeginInfo.html)** 구조체의 특정 파리미터를 사용하여 구성됩니다.

```cpp
VkRenderPassBeginInfo renderPassInfo = {};
renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
renderPassInfo.renderPass = renderPass;
renderPassInfo.framebuffer = swapChainFramebuffers[i];
```

첫번째 파라미터는 render pass 자신이고 그 다음은 바인딩 할 attachment입니다. 우리는 color attachment로 지정된 swap chain image 각각에 대해 framebuffer를 생성했었습니다.

```cpp
renderPassInfo.renderArea.offset = { 0, 0 };
renderPassInfo.renderArea.extent = swapChainExtent;
```

다음 두 파라미터는 렌더링 영역의 크기를 지정합니다. 렌더링 영역은 shader가 로드하고 저장할 영역을 지정합니다. 이 영역을 벗어나는 픽셀은 undefined 값을 가지게 됩니다. 이것(renderArea)는 최고의 성능을 위해서 attachment의 크기와 같아야 합니다.

```cpp
VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
renderPassInfo.clearValueCount = 1;
renderPassInfo.pClearValues = &clearColor;
```

마지막 두 파라미터는 color attachment의 load operation의  clear 값(**`VK_ATTACHMENT_LOAD_OP_CLEAR`**에서 사용되는)을 정의합니다. 간단히 100% 불투명도의 검정색으로 지정했습니다.

```cpp
vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
```

이제 render pass가 시작되었습니다. command를 기록하는 모든 함수는 그들이 가지고 있는 **[vkCmd](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmd.html)** 접두사로 알아볼 수 있습니다. 이 함수들은 리턴값은 모두 **`void`** 입니다. 때문에 기록을 완료할 때 까지 오류 처리가 없을 겁니다.

모든 command의 첫번째 파라미터는 항상 command를 기록할 command buffer입니다. 두번째 파라미터는 방금 구성한 render pass의 세부항목입니다. 마지막 파라미터는 render pass 내에서 drawing command가 어떻게 제공되는지를 제어합니다. 이는 두 값중 하나를 가질 수 있습니다.

- **`VK_SUBPASS_CONTENT_INLINE`** : render pass command가 primary command buffer 자체에 포함되며 secondary command buffer는 실행되지 않습니다.
- **`VK_SUBPASS_CONTENT_SECONDARY_COMMAND_BUFFER`** : render pass command가 secondary command buffer에서 실행됩니다.

우리는 secondary command buffer를 사용하지 않기 때문에 첫번째 옵션을 사용할 것입니다.

## **Basic drawing commands**

우리는 이제 graphics pipeline을 바인딩 할수 있습니다.

```cpp
vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
```

두번째 파라미터는 이것이 graphics pipeline 인지 compute pipeline인지 지정합니다. 이제 Vulkan에게 graphics pipeline에서 어떤 operation이 수행할지와 fragment shader에서 어떤 attachment를 사용할지 알려주었습니다. 그래서 이제 남은건 삼각형을 그리라고 말하는 것입니다.

```cpp
vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);
```

실제 **[vkCmdDraw](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdDraw.html)** 함수는 약간 허무하지만, 사전에 우리가 지정한 모든 정보로 인해서 매우 간단합니다. 이는 command buffer는 빼고 다음과 같은 파라미터를 가집니다.

- **`vertexCount`** : 우리가 vertex buffer를 사용하지 않기는 하지만, 엄밀히말해 draw를 위해서 3개의 vertex를 가지고 있습니다.
- **`instanceCount`** : instanced 렌더링에 사용되며, 원하지 않을 경우 **`1`** 을 사용합니다.
- **`firstVertex`** : vertex buffer에 대한 offset으로 사용되며 **`gl_VertexIndex`** 의 최소값을 정의합니다.
- **`firstInstance`** : instanced 렌더링에서 오프셋으로 사용되며 **`gl_InstanceIndex`** 의 최소값을 정의합니다.

## **Finishing up**

이제 render pass를 종료할 수 있습니다.

```cpp
vkCmdEndRenderPass(commandBuffers[i]);
```

그리고 command buffer 기록을 끝냈습니다.

```cpp
if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
	throw std::runtime_error("failed to record command buffer!");
}
```

다음 챕터에서 main loop을 위해 코드를 작성할 겁니다.

1. swap chain에서 image 획득
2. 올바른 command buffer 실행
3. 완료된 image를 swap chain에 반환

**[C++ code](https://vulkan-tutorial.com/code/14_command_buffers.cpp)** / **[Vertex shader](https://vulkan-tutorial.com/code/09_shader_base.vert)** / **[Fragment shader](https://vulkan-tutorial.com/code/09_shader_base.frag)**


### ref
https://dolong.notion.site/Drawing-a-triangle-Drawing-Command-buffers-6df254c6c58042a998ba3c6d27adf98f
