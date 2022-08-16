# Setup

pipeline 생성을 완료하기 전에 Vulkan에게 렌더링 하는 동안 사용할 framebuffer attachment들에 대해 알려줘야 합니다. 거기에 얼마나 많은 color/depth buffer가 있는지, 그들 각각이 얼마나 많은 sample이 사용되는지, 그들의 컨텐츠를 렌더링 작업을 하는 동안 어떻게 처리해야 하는지를 지정해야 합니다. 이 모든 정보가 ***render pass*** 오브젝트에 래핑됩니다.   
이 오브젝트를 위해 **`createRenderPass`** 함수를 생성합니다. 이 함수 호출을 **`initVulkan`** 함수안의 **`createGraphicPipeline`** 호출 전에 추가합니다.

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
}

...

void createRenderPass() {

}
```

### RenderPass
간단하게예를 들면, Depth ▶ G Buffer ▶ Shadow Depth ▶ Lighting Accumulation ▶ Translucency로 이어지는 deferred lighting 기법을 아래와 같은 그래프로 간단히 표현할 수 있습니다.  

![image](https://user-images.githubusercontent.com/16304843/184916857-0b386d88-d902-42b7-ab35-0b41a707ff11.png)  

 각 단계별로 사용하는 Attachment들이 다르며 그 Attachment간에는 종속성이 발생합니다.  
 Attachment는 실제 리소스에 대한 기술(description)이라 생각하면 됩니다.  

![image](https://user-images.githubusercontent.com/16304843/184917043-05d54fec-a709-4985-a8cf-a401784a5237.png)  

각 단계롤 Render Pass라 볼 수 있으며, 이것이 사용하는 Attachment들 간의 종속성에 의해 RenderPass의 종속성이 발생합니다.  


# Attachment Description

우리의 경우 swap chain의 이미지들 중 하나를 나타내는 단일 color buffer attachment만 있습니다.

```cpp
void createRenderPass() {
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
}
```



color attachment의 **`format`** 은 swap chain image들의 format과 일치해야 합니다. 그리고 multisampling 관련 작업은 아직 하지 않기 때문에 smaple은 `1`개를 유지합니다.

```cpp
colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
```

**`loadOp`** 와 **`stroeOp`** 는 attachment의 데이터가 렌더링 전에/렌더링 후에 무엇을 할지 결정합니다. 아래에 **`loadOp`** 를 위한 리스트가 있습니다.

- **`VK_ATTACHMENT_LOAD_OP_LOAD`** : 기존에 attachment에 있던 컨텐츠를 유지합니다.
- **`VK_ATTACHMENT_LOAD_OP_CLEAR`** : 시작시 값을 상수로 지웁니다.
- **`VK_ATTACHMNET_LOAD_OP_DONT_CARE`** : 기존 컨텐츠가 undefined가 됩니다: 우리는 이를 상관하지 않을 겁니다.

우리의 경우 새로운 프레임을 그리기 전에 framebuffer를 검정색으로 지우는 clear operation를 사용할 것입니다.

**`storeOp`** 를 위해서는 단 2개의 선택지만 있습니다.

- **`VK_ATTACHMENT_STROE_OP_STORE`** : 렌더링된 컨텐츠가 메모리에 저장되고 이후 이것을 읽을 수 있습니다.
- **`VK_ATTACHMENT_STORE_OP_DONT_CARE`** : 렌더링 작업 이후에 framebuffer의 컨텐츠는 undefined가 됩니다.

우리는 스크린에 렌더링 된 삼각형을 보는데 관심이 있으므로, 여기서는 store operation를 사용할 것입니다.

```cpp
colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
```

**`loadOp`** 와 **`storeOp`** 는 color와 depth 데이터에 적용되고 **`stencilLoadOp`** 와 **`stencilStoreOp`** 는 stencil data에 적용됩니다. 우리 어플리케이션은 stencil buffer로 어떤것도 하지 않으므로  loading과 storing의 결과는 관심없습니다.

```cpp
colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
```

Vulkan의 texture와 framebuffer는 특정 픽셀 포맷의 **[VkImage](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkImage.html)** 오브젝트로 표현됩니다. 하지만 메모리상의 픽셀 layout은 이미지로 하려고 하는 작업에 따라 변경될 수 있습니다.

가장 일반적인 layout은 다음과 같습니다.

- **`VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL`** : 이미지를 color attachment로 사용
- **`VK_IMAGE_LAYOUT_PRESENT_SRC_KHR`** : swap chain에서 표시할 이미지
- **`VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL`** : 메모리 복사 작업의 목적지로 사용할 이미지

우리는 이 주제를 texturing 챕터에서 좀 더 깊게 살펴볼 것입니다. 하지만 지금 당장 알아야할 중요한게 있습니다. 이미지는 다음에 수행할 작업에 적합한 특정한 레이아웃으로 전환되어야 한다는 것입니다.

**`initialLayout`** 은 render pass를 시작하기 전 상태의 image layout를 지정합니다. **`finalLayout`** 은 render pass가 끝났을때 자동적으로 전환될 layout을 지정합니다. **`intialLayout`** 에 **`VK_IMAGE_LAYOUT_UNDEFINED`** 을 사용한다는 의미는 image가 이전에 어떤 layout이었든지 상관없다는 뜻입니다. 이 특별한 값의 주의사항은 image의 내용이 보존되는 것을 보장해주지 않는다는 것입니다. 하지만 우리는 그걸 지울거라 문제가 되지 않습니다. 우리는 렌더링 후에 swap chain을 사용하여 image 프레젠테이션을 준비할거라 **`finalLayout`** 으로 **`VK_IMAGE_LAYOUT_PRESENT_SRC_KHR`** 를 사용합니다.

## **Subpasses and attachment reference**

단일 render pass는 여러개의 subpass로 구성됩니다. subpass는 이전 pass의 framebuffer 내용에 의존하는 후속 렌더링 작업입니다. 예를 들면 잇따라서 적용되는 일련의 post-processing 이펙트 같은 것이 있습니다. 이런 렌더링 operation들을 하나의 render pass에 그룹화할 경우 Vulkan은 operation들을 재정렬하고 더 나은 성능을 위하여 메모리 대역폭을 절약할 수 있습니다. 하지만 우리의 아주~ 첫번째 삼각형을 위해서는 단일 subpass를 유지합니다.

모든 subpass는 하나이상의 attachment를 참조합니다. 이 attachment는 우리가 이전 섹션에서 구조체를 사용하여 기술했습니다. 이 참조는 **[VkAttachmentReference](https://www.notion.so/VkAttachmentReference-Manual-Page-15c1681c14114231b4943fa52fdf8878)** 구조체 자체로 다음과 같습니다.

```cpp
VkAttachmentReference colorAttachmentRef = {};
colorAttachmentRef.attachment = 0;
colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
```

**`attachment`** 파라미터는 attachment description 배열의 인덱스로 참조할 attachment를 지정합니다. 우리의 배열은 단일 **[VkAttachmentDescription](https://www.notion.so/VkAttachmentDescription-Manual-Page-774a0dde223c41939b99b4b4f04349c9)**으로 구성되어 있기 때문에 인덱스는 **`0`**입니다. **`layout`**은 subpass가 이 참조를 사용하는 동안 attachment가 가질 layout를 지정합니다. Vulkan은 subpass가 시작되면 attachment를 이 layout으로 자동적으로 전환합니다. 우리는 attachment를 color buffer 같은 기능으로 사용할 예정이라 **`VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL`** layout이 이름에서도 암시하듯 우리에게 가장 좋은 성능을 제공해 줄겁니다.

subpass는 **[VkSubpassDescription](https://www.notion.so/VkSubpassDescription-Manual-Page-58ee91f712a84a689de125ea4268999e)** 구조체를 사용하여 기술됩니다.

```cpp
VkSubpassDescription subpass = {};
subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
```

Vulkan은 추후에 compute subpass도 지원할 예정이라 이게 graphics subpass라는 걸 명시해야 합니다. 그다음 color attachment의 참조를 지정합니다.

```cpp
subpass.colorAttachmentCount = 1;
subpass.pColorAttachments = &colorAttachmentRef;
```

이 배열의 attachment의 인덱스는 fragment shader에서 `layout(location = 0) out vec4 outColor`지시문으로 직접 참조됩니다!

아래의 다른 유형의 attachment들도 subpass에서 참조할 수 있습니다.

- **`pInputAttachments`** : shader에서 읽는 attachment
- **`pResolveAttachments`** : multisampling color attachment를 위해 사용하는 attachment
- **`pDepthStencilAttachment`** : depth & stencil 데이터를 위한 attachment
- **`pPreserveAttachments`** : 이 subpass에서 사용되지는 않지만 데이터를 보존해야하는 attachment





# **Render pass**

이제 attachment와 이를 참조하는 기본적인 subpass를 기술했으니 render pass 자체를 생성할 수 있습니다. **[VkRenderPass](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkRenderPass.html)** 오브젝트를 저장할 클래스 멤버를 생성하여 **`pipelineLayout`** 오브젝트 바로 위에 추가합니다.

```cpp
VkRenderPass renderPass;
VkPipelineLayout pipelineLayout;
```

render pass 오브젝트는 attachment 배열과 subpass을 사용하여 **[VkRenderPassCreateInfo](https://www.notion.so/VkRenderPassCreateInfo-Manual-Page-5f137856ca3b4755a5840471d21d7695)** 구조체를 채움으로써 생성할 수 있습니다. **[VkAttachmentReference](https://www.notion.so/VkAttachmentReference-Manual-Page-15c1681c14114231b4943fa52fdf8878)** 구조체는 이 배열(attachment 배열)의 인덱스를 사용하여 attachment를 참조합니다.

```cpp
VkRenderPassCreateInfo renderPassInfo = {};
renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
renderPassInfo.attachmentCount = 1;
renderPassInfo.pAttachments = &colorAttachment;
renderPassInfo.subpassCount = 1;
renderPassInfo.pSubpasses = &subpass;

if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
	throw std::runtime_error("failed to create render pass!");
}
```

pipeline layout과 같이 render pass는 프로그램 전반에 걸쳐 참조됩니다. 때문에 오직 프로그램의 끝에서만 정리되어야 합니다.

```cpp
void cleanup() {
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);
	...
}
```

많은 작업이었습니다. 하지만 다음장에서 이것들이 모두 모여 최종적으로 graphics pipeline 오브젝트를 만들것입니다!

**[C++ code](https://vulkan-tutorial.com/code/11_render_passes.cpp)** / **[Vertex shader](https://vulkan-tutorial.com/code/09_shader_base.vert)** / **[Fragment shader](https://vulkan-tutorial.com/code/09_shader_base.frag)**  

## ref

https://lifeisforu.tistory.com/462?category=837815  
