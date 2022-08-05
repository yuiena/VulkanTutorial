Swap chain 안에 있는 **[VkImage](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkImage.html)**를 사용하려면 render pipeline안에서 **[VkImageView](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkImageView.html)** 오브젝트를 생성해야 합니다. 

Image view는 말 그대로 `image에 대한 view` 입니다. Image view는 `image를 엑세스하는 방법과 엑세스 할 이미지의 부분을 기술`합니다. 예를들면 image를 mipmapping 없이(엑세스할 이미지 부분) 2D texture, depth texture로(image를 엑세스하는 방법) 간주해야 할 수 있습니다.

우리는 이번 챕터에서 swap chain에 있는 모든 image에 대한 기본적인 image view를 생성하는 **`createImageViews`** 함수를 작성할 겁니다. 그리고 나중에 그 image view를 color target으로 사용할 겁니다.

첫번째로 image view를 저장할 클래스 멤버를 추가합니다.

```
std::vector<VkImageView> swapChainImageViews;
```

**`createImageViews`** 함수를 생성하고 swap chain 생성 바로 뒤에서 호출합니다.

```
void initVulkan() 
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
}

void createImageViews() {
}
```

먼저 해야 할 일은 우리가 생성할 모든 image view에 맞게 리스트 사이즈를 변경하는 것입니다.

```
void createImageViews() 
{
	swapChainImageViews.resize(swapChainImages.size());
}
```

다음은 swap chain image 전체에 걸쳐 반복하는 루프를 설정합니다.

```
for (size_t i = 0; i < swapChainImages.size(); i++) {
}
```

image view를 생성하기 위한 파라미터는 **[vkImageViewCreateInfo](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkImageViewCreateInfo.html)** 구조체로 지정됩니다. 처음 몇개의 파라미터는 간단합니다.

```
VkImageViewCreateInfo createInfo = {};
createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
createInfo.image = swapChainImages[i];
```

**`viewType`**과 **`format`** 필드는 image 데이터를 해석하는 방법을 지정합니다. **`viewType`** 파라미터는 image를 1D texture, 2D texture, 3D texture, cube map 등으로 간주할 수 있도록 해줍니다.

```
createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
createInfo.format = swapChainImageFormat;
```

**`component`**필드는 color channel을 섞을 수 있도록 해줍니다. 예를 들어 단색 텍스쳐를 만들기 위해 모든 채널은 red channel로 매핑할 수 있습니다. 또한 **`0`**에서 **`1`**사이의 상수 값으로 channel에 매핑할 수도 있습니다. 우리는 기본 매핑을 유지할 것입니다.

```
createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
```

**`subresourceRange`** 필드는 image의 용도와 image의 어떤 부분을 엑세스 해야 하는지를 기술합니다. 우리 image는 어떤 mipmapping level이나 다중 layer 없이 color target으로만 사용될 것입니다.

```
createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
createInfo.subresourceRange.baseMipLevel = 0;
createInfo.subresourceRange.levelCount = 1;
createInfo.subresourceRange.baseArrayLayer = 0;
createInfo.subresourceRange.layerCount = 1;
```

만약 여러분이 입체 3D 어플리케이션을 작성하고 있다면 다중 layer가 있는 swap chain을 생성했을 겁니다. 그리고 여러 layer를 엑세스 하여 오른쪽/왼쪽 눈의 시야를 재현하는 각 image에 대하여 다중 image view를 생성할 수 있습니다.

image view를 생성하는 것은 이제 **[vkCreateImageView](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCreateImageView.html)**를 호출하는 일입니다.

```
if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
	throw std::runtime_error("failed to create image views!");
}
```

image와는 다르게 (정확히 swap chain image와는 다르게) image view는 우리가 명시적으로 생성했습니다. 때문에 프로그램의 끝에서 위와 비슷한 루프를 다시 만들어 이들을 정리해야 합니다.

```
void cleanup() {
	for (auto imageView : swapChainImageViews) {
		vkDestroyImageView(device, imageView, nullptr);
	}
	...
}
```

image view는 image를 texture로 사용하기에는 충분하지만, 아직 render target으로 사용할 준비가 완전히 되어 있진 않습니다. 이를 위해서는 frambuffer라고 알려진 간접적인 단계가 하나 더 남아 있습니다. 하지만 그 전에 graphics pipeline를 설정해야 합니다.

**[C++ code](https://vulkan-tutorial.com/code/07_image_views.cpp)**



#### ref

https://dolong.notion.site/Drawing-a-triangle-Presentation-Image-views-7bd8c418ecf34a81b45ce67f3a569faf
