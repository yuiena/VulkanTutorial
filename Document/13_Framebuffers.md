우리는 지난 몇 챕터에서 framebuffer에 대해 많이 이야기를 했고 swap chain image들과 같은 포맷의 단일 framebuffer를 예상하고 render pass를 설정하였습니다. 허나 아직 실제로 생성하지는 않았죠.

render pass를 생성하는 동안 지정한 attachment들은 **[VkFramebuffer](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkFramebuffer.html)** 오브젝트로 래핑하여 바인딩됩니다. framebuffer 오브젝트는 attachment를 나타내는 모든 **[VkImageView](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkImageView.html)** 오브젝트를 참조합니다. 우리의 경우에는 이 attachment는 오직 한개(color attachment) 뿐입니다. 허나 attachment를 위해 사용되는 image는 swap chain이 반환하는(presentaion을 위해 우리가 가져오는) image에 의존합니다. 이 말은 swap chain에 있는 모든 image들을 위해 framebuffer를 만들어야 하고 drawing time에 획득한 이미지에 부합하는 framebuffer를 사용한다는 것입니다.

이를 위해 framebuffer들을 저장할 또 다른 **`std::vector`** 클래스 멤버를 생성합니다.

```cpp
std::vector<VkFramebuffer> swapChainFramebuffers;
```

이 배열의 오브젝트를 생성하기 위해 새로운 함수는 **`createFramebuffers`** 를 만들고 **`initVulkan`** 함수에서 graphics pipeline 생성 바로 뒤에서 호출합니다.

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
}

...

void createFramebuffers() {

}
```

시작은 모든 framebuffer를 저장하기 위한 컨테이너를 리사이즈 하는 것 부터입니다.

```cpp
void createFramebuffers() {
	swapChainFramebuffers.resize(swapChainImageViews.size());
}
```

그 다음 image view들을 반복하면서 image view에서 framebuffer를 생성합니다.

```cpp
for (size_t i = 0; i < swapChainImageViews.size(); i++) {
	VkImageView attachments[] = {
		swapChainImageViews[i]
	};

	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = renderPass;
	framebufferInfo.attachmentCount = 1;
	framebufferInfo.pAttachments = attachments;
	framebufferInfo.width = swapChainExtent.width;
	framebufferInfo.height = swapChainExtent.height;
	framebufferInfo.layers = 1;

	if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
	throw std::runtime_error("failed to create framebuffer!");
	}
}
```

보다시피 framebuffer를 생성하는건 아주 쉽습니다. 첫번째로 지정이 필요한 것은 framebuffer가 호환해야 하는 **`renderPass`** 입니다. framebuffer는 호환되는 render pass만 사용할 수 있습니다. 이는 대충 동일한 갯수와 타입의 attachment를 사용해야 한다는 의미입니다.

**`attachmentCount`** 와 **`pAttachment`** 파라미터는 **[VkImageView](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkImageView.html)** 오브젝트(render pass의 **`pAttachment`** 배열에 있는 각각의 attachment description에 바인딩 되어야 하는)를 지정합니다.

**`width`**와 **`height`** 파라미터는 자동적으로 설명되며, **`layers`** 는 image 배열에 있는 layer의 수를 나타냅니다. 우리의 swap chain image들은 단일 image 이므로 layer 수는 **`1`**입니다.

framebuffer들은 그들의 기반인 image view들과 render pass 이후에 삭제되어야 합니다. 추가로 오직 렌더링이 끝난 다음에 이를 수행해야 합니다.

```cpp
void cleanup() {
	for (auto framebuffer : swapChainFramebuffers) {
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}
	...
}
```

우리는 이제 렌더링에 필요한 모든 오브젝트가지는 이정표에 도달했습니다. 다음 챕터에서 첫번째 실제 drawing command를 작성할 것입니다.

**[C++ code](https://vulkan-tutorial.com/code/13_framebuffers.cpp)** / **[Vertex shader](https://vulkan-tutorial.com/code/09_shader_base.vert)** / **[Fragment shader](https://vulkan-tutorial.com/code/09_shader_base.frag)**
