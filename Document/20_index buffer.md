## **Introduction**

여러분이 실제 어플리케이션에서 렌더링할 3D mesh는 여러개의 삼각형 간에서 종종 버텍스를 공유할 것입니다. 이것은 사각형 그리기 같은 간단한 작업에서도 발생하는 일입니다.

![image](https://user-images.githubusercontent.com/16304843/191026634-0471dc77-2cc5-4660-a2d6-e4ecc3aed5ca.png)

사각형 그리기는 두개의 삼각형을 가지는데 이는 6개의 버텍스를 사용하는 vertex buffer가 필요하다는 의미입니다. 문제는 두개의 버텍스 데이터가 중복되어 50%의 중복성을 야기한다는 것입니다. 이것은 버텍스들이 평균 3개의 삼각형에서 재사용되는 좀 더 복잡한 mesh에서는 문제를 더 나쁘게 합니다. 이 문제의 해결책은 ***index buffer***를 사용하는 것입니다.

index buffer는 근본적으로 vertex buffer에 대한 포인터의 배열입니다. 이를 통해 여러분은 vertex data를 재정렬하고 여러 버텍스에 대해 기존 데이터를 재사용할 수 있습니다. 위의 그림은 4개의 고유한  버텍스를 포함하는 vertex buffer가 있는 경우 index buffer가 사각형에 대해 어떻게 보이는지를 보여줍니다. 첫번째 3개의 인덱스는 우상단 삼각형을 정의하고 마지막 3개의 인덱스는  좌하단 삼각형의 버텍스를 정의합니다.

## **Index buffer creation**

이번 챕터에서 우리는 vertex data를 수정하고 그림에서 보이는 것 처럼 index data를 추가해서 사각형을 그릴 것입니다. 4개의 모서리를 표현하기 위해 vertex data를 수정합니다.

```cpp
const std::vector<Vertex> vertices = {
	{ { -0.5f, -0.5f },{ 1.0f, 0.0f, 0.0f } }, // red
	{ { 0.5f, -0.5f },{ 0.0f, 1.0f, 0.0f } }, // green
	{ { 0.5f, 0.5f },{ 0.0f, 0.0f, 1.0f } }, // blue
	{ { -0.5f, 0.5f },{ 1.0f, 1.0f, 1.0f } } // white
};
```

좌상단은 빨간색, 우상단은 녹색, 우하단은 파란색, 좌하단은 흰색입니다. 우리는 index buffer의 컨텐츠를 표현하기 위해 새로운 **`indices`** 배열을 추가할 것입니다. 우상단 삼각형과 좌하단 삼각형을 그리기 위해서는 그림의 인덱스와 일치해야 합니다.

```cpp
const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0
};
```

**`vertices`** 안의 항목 수에 따라 index buffer에 대해 **`uint16_t`** 나 **`unit32_t`** 를 사용할 수 있습니다. 우리는 65535개 보다 적은 고유 버텍스를 사용하고 있기 때문에 **`uint16_t`** 를 유지할 수 있습니다.

vertex data와 마찬가지로, index도 GPU가 그것을 읽을수 있도록 하기 위해 **[VkBuffer](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkBuffer.html)** 에 업로드하는게 필요합니다. index buffer를 위한 리소스를 저장하기 위해 두개의 새로운 클래스 멤버를 정의합니다.

```cpp
VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;
VkBuffer indexBuffer;
VkDeviceMemory indexBufferMemory;
```

우리가 이제 추가할 **`createIndexBuffer`** 는 대부분 **`createVertexBuffer`** 와 동일합니다.

```cpp
void initVulkan() {
	...
	createVertexBuffer();
	createIndexBuffer();
	...
}

...

void createIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

	copyBuffer(stagingBuffer, indexBuffer, bufferSize);
	
	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}
```

여기에는 오직 두개의 눈에 띄는 차이점만 존재합니다. **`bufferSize`** 는 이제 index의 갯수와 index type(**`uint16_t`** 혹은 **`uint32_t`**)의 곱과 같습니다. **`indexBuffer`**의 usage는 **`VK_BUFFER_USAGE_VERTEX_BUFFER_BIT`**가 아닌 **`VK_BUFFER_USAGE_INDEX_BUFFER_BIT`**여야 말이 됩니다. 그 외의 프로세스는 정확히 동일합니다. 우리는 **`indices`**의 컨텐츠를 복사하기 위한 staging buffer를 만들고 이것을 final device local index buffer에 복사합니다.

index buffer는 vertex buffer처럼 프로그램의 종료시점에 삭제되어야 합니다.

```cpp
void cleanup() {
	cleanupSwapChain();

	vkDestroyBuffer(device, indexBuffer, nullptr);
	vkFreeMemory(device, indexBufferMemory, nullptr);

	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);

	...
}
```

## **Using an index buffer**

drawing을 위한 index buffer의 사용에는 **`createCommandBuffers`** 의 두가지 변경이 포함됩니다. 우리는 첫번째로 vertex buffer에 대해 했던것 처럼 index buffer를 바인드해야 합니다. 다른점은 오직 하나의 index buffer만 가질 수 있다는 것입니다. 각 vertex attribute에 대해 서로다른 인덱스를 사용하는 것은 불행히도 불가능하기 때문에 하나의 attribute만 다를지라도 전체 vertex data를 복제해야 합니다.

```cpp
vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);
```

index buffer는 **[vkCmdBindIndexBuffer](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdBindIndexBuffer.html)**로 바인딩되며 이 함수는 index buffer, index buffer의 byte offset, index data의 타입을 파라미터로 가집니다. 앞서 언급했듯이, 가능한 타입은 **`VK_INDEX_TYPE_UINT16`**나 **`VK_INDEX_TYPE_UINT32`** 입니다.

index buffer를 바인딩 하는 것 만으로는 아직 아무것도 바뀌지 않습니다. 우리는 drawing command를 변경하여 Vulkan에게 index buffer를 사용한다고 말해야 합니다. **[vkCmdDraw](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdDraw.html)** 라인을 삭제하고 이를 **[vkCmdDrawIndexed](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdDrawIndexed.html)**로 변경합니다.

```cpp
vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
```

이 함수의 호출은 **[vkCmdDraw](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdDraw.html)** 와 매우 유사합니다. 첫번째 두 파라미터는 인덱스의 수와 인스턴스의 수를 지정합니다. 우리는 인스턴싱을 사용하지 않기 때문에 **`1`** 개의 인스턴스만 지정합니다. 인덱스의 수는 vertex buffer에 전달될 버텍스의 수를 나타냅니다. 다음 파라미터는 index buffer의 offset를 지정합니다. 이값을 **`1`**로 사용하면 그래픽 카드는 두번째 인덱스부터 읽기 시작합니다. 마지막에서 두번째 파라미터는 index buffer의 인덱스에 더할 offset을 지정합니다.  마지막 파라미터는 인스턴싱에 대한 offset을 지정하며 우리는 이를 사용하지 않습니다.

이제 프로그램을 실행하면 여러분은 아래와 같은 것을 보게될 것입니다.

![image](https://user-images.githubusercontent.com/16304843/191026611-c4e45500-c8f1-43d5-9d20-398ba2519218.png)


이제 여러분은 index buffer로 버텍스를 재사용 함으로써 메모리를 절약하는 방법을 알게되었습니다. 이는 복잡한 3D model을 로드할 이후 챕터에서 특히 중요해 질 것입니다.

이전 챕터에서 미리 언급했듯이, 여러분은 단일 메모리 할당에 buffer들과 같은 여러개의 리소스를 할당해야 합니다. 하지만 사실 여기서 한걸음 더 나아가야 합니다. **[Driver develpers recommand](https://developer.nvidia.com/vulkan-memory-management)** 에서는 여러분이 vertex나 index buffer 같은 여러개의 buffer들을 단일 **[VkBuffer](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkBuffer.html)**에 저장하고 **[vkCmdBindVertexBuffers](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdBindVertexBuffers.html)** 같은 command에서 offset을 사용하라고 권장합니다. 이 경우 여러분의 data는 좀더 cache 친화적이 되는 이점이 있습니다. 이들이 서로 가깝게 위치하기 때문이죠. 여러개의 리소스에 대한 메모리의 같은 청크를 재사용하는 것도 가능합니다. 만약 그것이 같은 render operation 중에 사용되지 않다면 말이죠. 물론 데이터가 새로 고쳐진 경우도 가능합니다. 이는 ***aliasing***이라고 알려져 있으며 일부 Vulkan 함수는 여러분이 이를 원할 경우 지정할 수 있는 명시적인 플래그를 가지고 있습니다.

**[C++ code](https://vulkan-tutorial.com/code/20_index_buffer.cpp)** / **[Vertex shader](https://vulkan-tutorial.com/code/17_shader_vertexbuffer.vert)** / **[Fragment shader](https://vulkan-tutorial.com/code/17_shader_vertexbuffer.frag)**
