
# Introduction
vertex buffer는 현재 잘 동작합니다. 하지만 CPU에서 우리가 vertex buffer에 액세스할 수 있도록 해주는 메모리 유형은 그래픽 카드가 읽기에 가장 최적인 메모리 유형은 아닐 수 있습니다. 가장 최적의 메모리는 `VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT` flag를 가지며 이는 일반적으로 그래픽 카드 전용으로 CPU에 의해 액세스가 불가능합니다.   
이 챕터에서 우리는 두 개의 vertex buffer를 생성할 것입니다. vertex 배열에서 데이터를 업로드 하기위한 CPU 액세스 가능 메모리인 **staging buffer**와 device local memory상의 vertex buffer 입니다. 그리고나서 우리는 buffer copy command를 사용하여 data를 staging buffer에서 실제 vertex buffer로 이동시킬 것입니다.

## **Transfer queue**

buffer copy command는 transfer operation을 지원하는 queue family가 필요합니다. 이것은 **`VK_QUEUE_TRANSFER_BIT`** 를 사용하여 나타냅니다. 좋은 소식은 **`VK_QUEUE_GRAPHICS_BIT`** 나 **`VK_QUEUE_COMPUTE_BIT`** 기능이 있는 어떤 queue family도 이미 암묵적으로 **`VK_QUEUE_TRANSFER_BIT`** operation을 지원한다는 것입니다. 이 경우 implementation에서 **`queueFlags`** 에서 명시적인 리스트가 필요하지 않습니다.

여러분이 도전을 좋아하신다면, transfer operation에 특화된 다른 queue family를 사용하려고 할 수도 있습니다. 이를 위해서는 여러분의 프로그램에 아래와 같은 변경을 적용해야 할것입니다.

- **`QueueFamilyIndices`**와 **`findQueueFmailies`** 를 수정하여 명시적으로 **`VK_QUEUE_GRAPHICS_BIT`** 가 아닌 **`VK_QUEUE_TRANSFER_BIT`** 를 사용하는 queue family를 찾습니다.
- **`createLogicalDevice`**를 수정하여 transfer queue의 핸들을 요청합니다.
- transfer queue family상에서 제출할 command buffer를 위한 두번째 command pool를 생성합니다.
- 리소스의 **`sharingMode`** 를 수정하여 **`VK_SHARING_MODE_CONCURRENT`** 가 되게 하고 graphics와 transfer queue family 둘다에 지정합니다.
- **[vkCmdCopyBuffer](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdCopyBuffer.html)**(이것은 우리가 이번 챕터에서 사용할 것입니다) 같은 transfer command들을 graphics queue가 아닌 transfer queue에 제출합니다.

약간의 작업이 있긴 하지만, 이것은 여러분에게 queue family 간에 리소스를 동기화 하는 방법에 대해 많이 가르쳐 줍니다.

## **Abstracting buffer creation**

이 챕터에서는 여러개의 buffer를 생성할 것이기 때문에 buffer 생성을 helper 함수로 이동시키는 것은 좋은 생각입니다. 새로운 함수인 **`createBuffer`** 를 생성하고 **`createVertexBuffer`** 의 코드를 이곳으로 이동시킵니다(매핑 부분은 제외).

```cpp
void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(device, buffer, bufferMemory, 0);
}
```

buffer size, memory properties, usage에 대한 파라미터를 추가해서 우리가 이 함수를 여러가지 유형의 buffer를 생성하는데 사용할 수 있도록 합니다. 마지막 두 파라미터는 핸들을 기록하기 위한 output 변수입니다.

이제 **`createVertexBuffer`** 에서 buffer 생성과 메모리 할당 부분을 지우고 그 대신 **`createBuffer`** 를 호출할 수 있습니다.

```cpp
void createVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexBuffer, vertexBufferMemory);

	void* data;
	vkMapMemory(device, vertexBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, vertexBufferMemory);
}
```

프로그램을 실행하여 vertex buffer가 제대로 동작하는지 확인하십시오.

## **Using a staging buffer**

이제 우리는 **`createVertexBuffer`** 를 수정해서 임시 buffer는 host visible buffer로만 사용하고 실제 vertex buffer로 device loacal을 사용합니다.

```cpp
void createVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

}
```

우리는 이제 vertex data 매핑과 복사를 위해 **`stagingBufferMemory`** 와 함께 새로운 **`stagingBuffer`** 를 사용하고 있습니다. 이 챕터에서 우리는 두개의 새로운 buffer usage flag를 사용할 것입니다.

- **`VK_BUFFER_USAGE_TRANSFER_SRC_BIT`**: Buffer가 memory transfer operation에서 source로 사용될 수 있습니다.
- **`VK_BUFFER_USAGE_TRANSFER_DST_BIT`**: Buffer가 memory transfer operation에서 destination으로 사용될 수 있습니다.

**`vertexBuffer`** 는 이제 device local 메모리 유형으로 할당되며 이것은 일반적으로 **[vkMapMemory](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkMapMemory.html)** 를 사용할 수 없음을 의미합니다. 그러나 우리는 **`stagingBuffer`** 에서 **`vertexBuffer`**로 데이터를 복사할 수 있습니다. **`stagingBuffer`** 에 transfer source flag와 **`vertexBuffer`** 에는 vertex buffer usage flag와 함께 transfer destination flag을 지정하여 우리가 이를 수행하려는 의도를 나타내야 합니다.

이제 우리는 한 buffer에서 다른 buffer로 컨텐츠를 복사하는 **`copyBuffer`** 라는 함수를 작성할 것입니다.

```cpp
void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
}
```

memory transfer operation은 drawing command 처럼 command buffer를 사용하여 동작합니다. 따라서 우리는 먼저 임시 command buffer를 생성해야 합니다. 여러분은 이런 종류의 짧은 생명주기를 가지는 buffer를 위해  분리된 command pool을 생성하고 싶을 수도 있습니다. 왜냐면 implementation이 메모리 할당 최적화를 적용할 수도 있기 때문입니다. 이 경우 command pool을 생성하는 동안 **`VK_COMMAND_POOL_CREATE_TRANSFER_BIT`**을 사용해야 합니다.

```cpp
void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
}
```

그리고 바로 command buffer 기록을 시작합니다.

```cpp
VkCommandBufferBeginInfo beginInfo = {};
beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

vkBeginCommandBuffer(commandBuffer, &beginInfo);
```

우리는 command buffer를 한번만 쓸것이고 copy operation이 수행을 완료할 때 까지 함수의 반환을 대기할 것입니다. **`VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT`** 를 사용하여 driver에게 우리의 의도를 알려주는 것은 좋은 습관입니다.

```cpp
VkBufferCopy copyRegion = {};
copyRegion.srcOffset = 0; // Optional
copyRegion.dstOffset = 0; // Optional
copyRegion.size = size;
vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
```

buffer의 컨텐츠는 **[vkCmdCopyBuffer](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdCopyBuffer.html)** command를 사용하여 전송됩니다. 이것은 source와 destination buffer, 복사할 범위를 배열을 인자로 받습니다. 범위는 **[VkBufferCopy](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkBufferCopy.html)** 구조체안에 정의되어 있고 source buffer offset, destination buffer offset, size로 구성됩니다. **[vkMapMemory](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkMapMemory.html)** command와는 다르게 여기선 **`VK_WHOLE_SIZE`** 지정이 불가능합니다.

```cpp
vkEndCommandBuffer(commandBuffer);
```

이 command buffer는 오직 copy command만 포함되므로 우리는 위의 작업 후 바로 기록을 중단 할 수 있습니다. 이제 command buffer를 실행하여 전송을 완료하십시오.

```cpp
VkSubmitInfo submitInfo = {};
submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
submitInfo.commandBufferCount = 1;
submitInfo.pCommandBuffers = &commandBuffer;

vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
vkQueueWaitIdle(graphicsQueue);
```

draw command와는 다르게 이번에는 우리가 우리가 기다려야 할 이벤트가 없습니다. 우리는 즉시 buffer에서 전송이 수행되길 원합니다. 이 전송이 완료되는 것을 기다리는 데는 다시 두가지 방법이 있습니다. 우리는 fence를 사용하고 **[vkWaitForFences](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkWaitForFences.html)** 로 대기할 수 있고 혹은 **[vkQueueWaitIdle](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkQueueWaitIdle.html)** 를 사용하여 간단히 transfer queue가 idel이 되는 것을 기다릴 수도 있습니다. fence는 여러분이 여러개의 전송을 동시에 예약하고 그 모든것이 완료되는 것을 대기할수 있게 해줍니다. 한번에 하나씩 수행하지 않고 말이죠. 그것은 driver가 최적화할 좀 더 많은 기회를 제공합니다.

```cpp
vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
```

transfer operation에 사용한 command buffer를 삭제하는 것을 잊지 마십시오.

우리는 이제 vertex data를 device local buffer로 이동시키기 위해서 **`createVertexBuffer`**함수에서 **`copyBuffer`**를 호출할 수 있습니다.

```cpp
createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
```

staging buffer에서 device buffer로의 데이터 복사 이후 우리는 이를 해제해야 합니다.

```c
...

    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}
```

프로그램을 실행하여 익숙한 삼각형이 다시 보이는지를 확인하십시오. 개선점은 지금 바로 보이지는 않겠지만 vertex data는 이제 고성능 메모리에 로드됩니다. 이것은 우리가 좀 더 복잡한 geometry를 렌더링하기 시작할 때 중요할 것입니다.

## **Conclusion**

실제 어플리케이션에서 여러분은 모든 개별 buffer에 대해 실제로 **[vkAllocationMemory](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkAllocateMemory.html)** 호출하면 안된다는 점에 유의하십시오. 동시 메모리 할당의 최대 갯수는 **`maxMemoryAllocationCount`** 로 physical device 제한에 의해 제한됩니다. 이는 NVIDIA GTX 1080같은 하이엔드 하드웨어에서도 **4096**까지 낮아질 수 있습니다. 동일 시점에서 많은 수의 오브젝트에 대한 메모리 할당의 올바른 방법은 여러분이 많은 함수에서 봤던 **offset** 파라미터를 이용하여 여러 오브젝트를 단일 할당에 분할하는 기능을 가진custom allocator를 생성하는 것입니다.

여러분은 이러한 allocator를 스스로 구현하거나 GPUOpen initiative에서 제공하는 **[VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)** 라이브러리를 사용하실 수 있습니다. 하지만 이 튜토리얼에서는 모든 리소스에 대해 개별적인 할당을 해도 괜찮습니다. 왜냐면 지금은 이런 제약에 도달하지 않을 것이기 때문입니다.

**[C++ code](https://vulkan-tutorial.com/code/19_staging_buffer.cpp)** / **[Vertex shader](https://vulkan-tutorial.com/code/17_shader_vertexbuffer.vert)** / **[Fragment shader](https://vulkan-tutorial.com/code/17_shader_vertexbuffer.frag)**
