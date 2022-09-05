
# Introduction
Vulkan에서 buffer는 그래픽 카드가 읽을 수 있는 임의의 데이터를 저장하는데 사용되는 메모리 영역입니다. buffer는 데이터를 저장하는데 사용되는 것 뿐만 아니라 이후 챕터에서 알아보게 될 많은 다른 목적으로도 사용됩니다.
우리가 지금까지 다룬 Vulkan object와는 다르게, **buffer는 자기 자신을 위해서 자동적으로 메모리를 할당하지 않습니다.**  
이전 챕터들의 작업에서 Vulkan API는 프로그래머가 거의 모든 것을 컨트롤 할 수 있다는 것을 보여주었고 메모리 관리 또한 그런 것들 중 하나입니다.

# Buffer creation
새로운 `createVertexBuffer` 함수를 생성하고 이를 `initVulkan`에서 createCommandBuffer 바로 전에 호출합니다.

```
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
    createVertexBuffer();
    createCommandBuffers();
    createSyncObjects();
}

...

void createVertexBuffer() {

}
```

buffer를 생성하기 위해서는 `VkBufferCreateInfo` 구조체를 채우는 것이 필요합니다.

```
VkBufferCreateInfo bufferInfo = {};
bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
bufferInfo.size = sizeof(vertices[0]) * vertices.size();
```

struct의 첫번째 필드는 `size`입니다. 이것은 buffer의 byte 크기를 지정합니다. vertex data 의 byte 크기 계산은 간단하게 `sizeof`를 사용합니다.

```
bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
```
두번째 필드는 `usage`로써 buffer의 데이터가 어떤 용도로 사용되는지를 나타냅니다. bitwise or를 사용하여 여러개의 용도를 지정하는 것이 가능합니다. 우리 경우에는 vertex buffer일 것이므로 다른 유형의 사용법은 이후 챕터에서 알아볼 것입니다.

```
bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
```

swap chain에서의 image와 마찬가지로 buffer 또한 특정 queue family의 소유가 되거나 같은 시점에 여러곳에서 공유될 수 있습니다. buffer는 오직 graphics queue에서만 사용될 것이므로 우리는 exclusive access를 유지할 수있습니다.

`flag` 파라미터는 sparse buffer memory를 구성하는데 사용됩니다. 이것은 지금은 크게 관련이 없습니다. 우리는 이것을 기본값 `0`으로 남겨둘 것입니다.  
우리는 이제 vkCreateBuffer를 사용하여 buffer를 생성할 수 있습니다. buffer handle을 유지하기 위한 클래스 멤버를 정의하고 이를 vertexBuffer라고 부르겠습니다.

```
VkBuffer vertexBuffer;

...

void createVertexBuffer() {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(vertices[0]) * vertices.size();
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create vertex buffer!");
    }
}
```
buffer는 프로그램의 종료때까지 rendering command들에서 사용이 가능해야 하고, buffer는 swap chain에 의존하지 않습니다. 따라서 우리는 원래의 `cleanup` 함수에서 이를 제거할 것입니다.

```
void cleanup() {
	cleanupSwapChain();

	vkDestroyBuffer(device, vertexBuffer, nullptr);
	...
}
```

# Memory requirements
buffer가 생성되었습니다. 하지만 이 buffer에는 아직 실제로 어떤 메모리도 지정되지 않았습니다. buffer에 메모리 할당을 위한 첫번째 단계는 적절한 이름인 `vkGetBufferMemoryRequirements` 함수를 사용하여 memory requirement를 질의하는 것입니다.

```
VkMemoryRequirements memRequirements;
vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);
```

`VkMemoryRequirements` 구조체는 3개의 필드가 있습니다.

- size : 요구하는 메모리의 양의 byte 크기, bufferInfo.size와는 다를 수 있습니다.
- aligment : 할당된 메모리 영역에서 buffer가 시작되는 byte offset으로 bufferInfo.usage와 bufferInfo.flags에 의존합니다.
- memoryTypeBits : buffer에 적절한 메모리 유형의 Bit field

그래픽 카드는 할당을 위한 서로 다른 메모리 유형을 제공할 수있습니다. 각 메모리 유형은 허용되는 operation이나 성능 특성에 따라 다릅니다. 우리는 buffer의 요구사항과 우리 어플리케이션의 요구사항을 취합하여 사용할 올바른 유형의 메모리를 찾아야합니다.
이를 위해 `findMemoryType`이라는 새로운 함수를 생성합니다.

```
uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
}
```

먼저 우리는 vkGetPhysicalDeviceMemoryProperties를 사용하여 사용가능한 메모리 유형에 대한 정보를 쿼리해야 합니다.

```
VkPhysicalDeviceMemoryProperties memProperties;
vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
```

**[VkPhysicalDeviceMemoryProperties](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkPhysicalDeviceMemoryProperties.html)** 구조체에는 두개의 배열(**`memoryTypes`**, **`memoryHeaps`**)이 있습니다. Memory heaps는 전용 VRAM이나 VRAM이 부족할 때 RAM상의 swap space 같은 별개의 메모리 리소스입니다. 이 heap에는 서로다른 메모리 유형이 존재합니다. 지금 당장은 heap이 아니라 메모리 유형에 대해서만 고려할 것이지만, 여러분이 이것이 성능에 영향을 미칠거라고 상상할 수 있습니다.

먼저 buffer 자신에 알맞은 메모리 유형을 찾아봅시다.

```
for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
	if (typeFilter & (1 << i)) {
		return i;
	}
}

throw std::runtime_error("failed to find suitable memory type!");
```

**`typeFilter`** 파라미터는 알맞은 메모리 유형의 bit field를 지정하기 위해서 사용됩니다. 이 말은 우리가 간단히 이를 반복하고 대응되는 bit가 **`1`**로 세팅되었는지 체크하는 것으로 알맞은 메모리의 인덱스를 찾을 수 있다는 것입니다.

하지만, 우리는 그저 vertex buffer에 알맞은 메모리 유형에만 관심있는 것은 아닙니다. 우리는 우리의 vertex data를 메모리에 쓰는것도 가능해야 합니다. **`memoryTypes`** 배열은 각 메모리 유형의 heap과 properties을 지정하는 **[VkMemoryType](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkMemoryType.html)** 구조체로 구성됩니다. properties는 "map이 가능하여 우리가 CPU에서 이것을 write 할 수 있는 것"과 같은 메모리의 특별한 기능을 정의합니다. 이 프로퍼티은 **`VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT`**로 표시되지만 우리는 **`VK_MEMORY_PROPERTY_HOST_COHERENT_BIT`** 프로퍼티의 사용도 필요합니다. 우리가 메모리를 메핑할 때 왜 이게 필요한지 볼 수 있을 것입니다.

우리는 이제 이 프로퍼티의 지원도 체크하기 위해 loop를 수정할 수 있습니다.

```cpp
for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
	if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
		return i;
	}
}
```

우리는 하나 이상의 프로퍼티를 원할 수 있기 때문에 bitwise AND의 결과가 단지 non-zero가 아니라 원하는 프로퍼티의 bit field와 같은지를 체크해야 합니다. 필요로 하는 모든 프로퍼티를 가진, 버퍼에 대한 알맞은 메모리 유형이 존재하면 그 인덱스를 리턴하고 그렇지 않으면 예외를 던집니다.

## **Memory Allocation**

이제 우리에게는 올바른 메모리 유형을 결정할 수 있는 방법이 있으므로 **[VkMemoryAllocateInfo](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkMemoryAllocateInfo.html)** 구조체를 채움으로써 실제로 메모리를 할당 할 수 있습니다.

```cpp
VkMemoryAllocateInfo allocInfo = {};
allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
allocInfo.allocationSize = memRequirements.size;
allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
```

메모리 할당은 이제 size와 type를 지정하는 것처럼 간단하며 두가지 모두 vertex buffer의 메모리 요구사항과 원하는 프로퍼티로 부터 얻어집니다. 메모리 핸들을 저장할 클래스 멤버를 생성하고 **[vkAllocateMemory](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkAllocateMemory.html)**를 사용하여 이를 할당합니다.

```cpp
VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;

...

if (vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
	throw std::runtime_error("failed to allocate vertex buffer memory!");
}
```

메모리 할당이 성공하면 **[vkBindBufferMemory](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkBindBufferMemory.html)**를 사용하여 이 메모리를 버퍼와 연결할 수 있습니다.

```cpp
vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);
```

처음 3개의 파라미터는 스스로를 설명하고 있고, 네번째 파라미터는 메모리 영역 내의 오프셋입니다. 이 메모리는 해당 vertex buffer에 특화되어 할당되었기 때문에 오프셋은 간단이 `0`입니다. 오프셋이 **`0`**이 아니면 이것은 **`memRequirement.aligment`**로 나눌수 있어야 합니다.

물론, C++의 동적 메모리 할당처럼 메모리는 어느 시점에서는 해제되어야 합니다. buffer object에 바인딩 된 메모리는 buffer가 더이상 사용되지 않을 때 해제되어야 할것이기 때문에 buffer가 폐기된 후에 이것을 해제합니다.

```cpp
void cleanup() {
	cleanupSwapChain();

	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);
```

## **Filling the vertex buffer**

이제 vertex data를 buffer에 복사할 시간입니다. 이것은 **[vkMapMemory](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkMapMemory.html)**를 사용하여 CPU accessible memory에 **[mapping the buffer memory](https://en.wikipedia.org/wiki/Memory-mapped_I/O)**를 함으로써 수행됩니다.

```cpp
void* data;
vkMapMemory(device, vertexBufferMemory, 0, bufferInfo.size, 0, &data);
```

이 함수는 offset과 size로 정의된 지정된 메모리 리소스의 영역에 엑세스 할 수 있도록 해줍니다.  여기서 offset과 size는 각각 **`0`**과 **`bufferInfo.size`**입니다. 전체 메모리를 매핑하기 위해 특정한 값이 **`WK_WHOLE_SIZE`**를 지정하는 것도 가능합니다. 마지막에서 두번째 파라미터는 flag를 지정하는데 사용할 수 있지만 현재 API에서는 아직 사용가능하지 않습니다. 이것은 **`0`**으로 설정해야 합니다. 마지막 파라미터는 매핑된 메모리에 대한 포인터의 출력을 지정합니다.

```cpp
void* data;
vkMapMemory(device, vertexBufferMemory, 0, bufferInfo.size, 0, &data);
memcpy(data, vertices.data(), (size_t)bufferInfo.size);
vkUnmapMemory(device, vertexBufferMemory);
```

이제 여러분은 vertex data를 매핑된 메모리로 **`memcpy`** 할 수 있고 그후 **[vkUnmapMemory](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkUnmapMemory.html)** 를 사용하여 이를 다시 unmap 합니다. 불행히도 driver는 예를 들어 caching 같은 이유로 data를 buffer memory에 즉각적으로 복사하지는 않을 것입니다. buffer에 대한 write가 매핑된 메모리에 아직 보이지 않을 수도 있습니다. 이 문제를 다루는데는 두가지 방법이 있습니다.

- **`VK_MEMORY_PROPERTY_HOST_COHERENT_BIT`** 로 표시된 host coherent memory heap를 사용
- 매핑된 메모리에 write 후 **[vkFlushMappedMemoryRanges](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkFlushMappedMemoryRanges.html)** 를 호출하고, 매핑된 메모리를 읽기 전에 **[vkInvalidateMappedMemoryRanges](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkInvalidateMappedMemoryRanges.html)** 를 호출합니다.

우리는 첫번째 접근을 사용했습니다. 이는 매핑된 메모리가 항상 할당된 메모리의 컨텐츠와 일치함을 보장합니다. 이것은 명식적인 flushing보단 나쁜 성능을 초래할 수 있다는 것을 명심하십시오. 하지만 우리는 다음 챕터에서 그것이 왜 중요하지 않은지를 볼 것입니다.

memory range를 flushing하거나 coherent memory heap을 사용한다는 것은 driver가 우리의 buffer에 대한 write를 안다는 것을 의미하지만, 아직 그것이 GPU상에 실제로 보여진다는 것을 의미하지는 않습니다. GPU로의 data 전송은 백그라운드에서 발생하는 operation이며 다음 **[vkQueueSubmit](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkQueueSubmit.html)** 호출을 기준으로 완료가 보장된다고 스펙 문서에서 **[간단히 이야기](https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#synchronization-submission-host-writes)** 하고 있습니다.

# Binding the vertex buffer

이제 남은 것은 rendering operation 동안 vertex buffer를 바인딩하는 것 뿐입니다. 우리는 **`createCommandBuffers`** 함수를 확장하여 이를 수행할 것입니다.

```cpp
vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

VkBuffer vertexBuffers[] = { vertexBuffer };
VkDeviceSize offsets[] = { 0 };
vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(vertices.size()), 1, 0, 0);
```

**[vkCmdBindVertexBuffer](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdBindVertexBuffers.html)** 함수가 바인딩을 위해 vertex buffer를 바인드하는데 사용됩니다. 이전 챕터에서 설정한것 처럼요. command buffer 외에 처음 두 파라미터는 우리가 vertex buffer에 지정한 offset과 binding 수를 지정합니다. 마지막 두 파라미터는 바인드할 vertex buffer 배열과 vertex data에서 read를 시작할 바이트 오프셋을 지정합니다. 여러분은 또한 **[vkCmdDraw](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdDraw.html)** 를  변경하여 하드코딩 된 숫자 **`3`** 대신 buffer내의 vertex 수를 전달해야 합니다.

이제 프로그램을 실행하면 여러분은 익숙한 삼각형이 다시 보일 것입니다.

![image](https://user-images.githubusercontent.com/16304843/188454448-ba8052d3-f19a-4684-b9f6-7fab9e0f1d54.png)

`vertices` 배열을 수정해서 상단 vertex의 색상을 변경해 보십시오.

```cpp
const std::vector<Vertex> vertices = {
	{ { 0.0f, -0.5f },{ 1.0f, 1.0f, 1.0f } },
	{ { 0.5f, 0.5f },{ 0.0f, 1.0f, 0.0f } },
	{ { -0.5f, 0.5f },{ 0.0f, 0.0f, 1.0f } }
};
```

프로그램을 다시 실행하면 아래와 같이 보일 것입니다.

![image](https://user-images.githubusercontent.com/16304843/188454483-c4bb706c-bf51-4dca-b869-265841d67613.png)

다음 챕터에서 우리는 vertex buffer에 vertex data를 복사하는 또 다른 방법(더 좋은 성능을 보여주지만 좀 더 많은 작업이 필요한)을 보게 될 것입니다.

**[C++ code](https://vulkan-tutorial.com/code/18_vertex_buffer.cpp)** / **[Vertex shader](https://vulkan-tutorial.com/code/17_shader_vertexbuffer.vert)** / **[Fragment shader](https://vulkan-tutorial.com/code/17_shader_vertexbuffer.frag)**

![image](https://user-images.githubusercontent.com/16304843/188454525-5768ed01-debd-470f-817a-2f8dc265536b.png)


## ref
https://dolong.notion.site/Vulkan-Programming-Tutorial-2021-11-43e085f3906a401ea0bc4e9710139116?p=898d6e0c70204e7ab531b4a118bea8f1&pm=s
