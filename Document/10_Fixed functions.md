오래전 graphics API들은 graphics pipeline의 대부분의 단계에 대해 기본 상태를 제공했었습니다. 

Vulkan에서는 Viewport 크기에서 부터 Color blending 기능까지 모든 것을 명시적으로 해야 합니다. 이번 챕터에서는 이 fixed-function operation들을 구성하기 위한 모든 구조체들을 채울 것입니다.



# Vertex input

**[VkPipelineVertexInputStateCreateInfo](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkPipelineVertexInputStateCreateInfo.html)** 는 vertex shader에 전달되는 vertex 데이터의 포맷을 기술합니다. 대략 2가지 방법으로 이것을 기술합니다.

- Bindings : 데이터 간의 간격과 per-vertex 인지 per-instance(**[instancing](https://en.wikipedia.org/wiki/Geometry_instancing)** 참조) 인지 여부
- Attribute descriptions : vertex shader에게 전달된 attribute의 타입으로 이들을 어디에서, 어떤 오프셋으로 로드 할지를 바이딩 합니다.

우리는 vertex shader에 vertex 데이터를 직접 하드코딩할거라, 현재 로딩할 vertex 데이터는 없다고 지정하도록 구조체를 채울겁니다. vertex buffer 챕터에서 이를 다시한번 살펴볼 것입니다.

```cpp
VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
vertexInputInfo.vertexBindingDescriptionCount = 0;
vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
vertexInputInfo.vertexAttributeDescriptionCount = 0;
vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional
```

**`pVertexBindingDescriptions`** 와 **`pVertexAttributeDescriptions`** 멤버는 vertex data 로딩을 위해서 앞서 언급한 구조체 배열을 가리킵니다. 이 구조체를 **`createGraphicsPipeline`** 안에 **`shaderStage`** 배열 바로 뒤에 추가하십시오.



# Input assembly

**[VkPipelineInputAssemblyStateCreateInfo](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkPipelineInputAssemblyStateCreateInfo.html)** 구조체는 두가지를 기술합니다: Vertex들로 어떤 종류의 geometry를 그릴것이냐와 primitive restart가 활성화 되었는가 하는 것입니다. 

전자는 **`topology`** 멤버로 지정되며 아래와 같은 갓을 가질 수 있습니다.

- **`VK_PRIMITIVE_TOPOLOGY_POINT_LIST`** : vertex들의 포인트들
- **`VK_PRIMITIVE_TOPOLOGY_LINE_LIST`** : 재사용하지 않는 모든 2개의 vertex로 이루어진 선
- **`VK_PRIMITIVE_TOPOLOGY_LINE_STRIP`** : 모든 선의 끝 vertex가 다음 선의 시작 vertex로 사용
- **`VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST`** : 재사용하지 않은 모든 3개의 vertex로 이루어진 삼각형
- **`VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP`** : 모든 삼각형의 두번째 세번째 vertex가 다음 상각형의 첫 두 vertex로 사용

일반적으로, vertex는 vertex buffer에서 연속된 순서의 index를 사용하여 로드됩니다. 

하지만 *element buffer*를 사용하여 자체적으로 사용할 index를 지정할 수도 있습니다. 이를 통해 vertex 재사용 같은 최적화를 수행할 수 있습니다. **`primitiveRestartEnable`** 멤버를 **`VK_TRUE`** 로 설정하면 특수 index인 **`0xFFFF`** 나 **`0xFFFFFFFF`** 를 사용하여 **`_STRIP`** topology 모드에서 선과 삼각형을 끊을 수 있습니다.

우리는 이 튜토리얼 전체에 걸쳐 삼각형을 그릴 계획이라 구조체를 위한 다음의 데이터를 유지합니다.

```cpp
VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
inputAssembly.primitiveRestartEnable = VK_FALSE;
```



# Viewports and scissors

viewport는 기본적으로 framebuffer에서 렌더링되어 출력되는 지역에 대해 기술합니다. 이는 거의 대부분 `(0,0)`에서 `(width, height)`이고 이 튜토리얼에서도 같은 경우입니다.

```cpp
VkViewport viewport = {};
viewport.x = 0.0f;
viewport.y = 0.0f;
viewport.width = (float)swapChainExtent.width;
viewport.height = (float)swapChainExtent.height;
viewport.minDepth = 0.0f;
viewport.maxDepth = 1.0f;
```

swap chain과 그의 image들의 크기는 윈도우 **`WIDTH`**, **`HEIGHT`** 와 다를 수 있습니다. swap chain image들은 후에 framebuffer로 사용될 것이라 우리는 이 크기를 유지해야 합니다.

**`minDepth`** 와 **`maxDepth`** 값은 framebuffer를 위한 depth 값의 범위를 지정합니다. 

이 값은 **`[0.0f, 1.0f]`** 범위를 가져야 합니다만 **`minDepth`** 가 **`maxDepth`** 보다 클 수도 이습니다. 뭔가 특별한 작업을 하지 않는한 **`0.0f`** 와 **`1.0f`** 의 표준 값을 유지해야 합니다.

viewport가 image에서 framebuffer로의 transformation을 정의하고, **scissor 사각형** 은 `실제로 픽셀이 저장될 영역을 정의`합니다.  **scissor 사각형을 벗어나는 모든 픽셀은 rasterizer에 의해 버려집니다**. 

이 기능은 transformation 이라기 보단 필터에 가깝습니다. 다른 점은 아래에 이미지화 했습니다. 

왼쪽 scissor 사각형은 scissor 사각형이 viewport 보다 큰 이미지에 대한 결과일 수 있는 여러가지 가능성 중에 하나임에 주의하시기 바랍니다.



![img](https://dolong.notion.site/image/https%3A%2F%2Fs3-us-west-2.amazonaws.com%2Fsecure.notion-static.com%2F35754dc9-89fb-4bf0-8f77-57c6c37bd272%2Fviewports_scissors.png?table=block&id=6cc13d84-b308-4156-bf8d-d883ae16d5e8&spaceId=09757d3e-a434-430f-8919-d37a4c3bb1a4&width=960&userId=&cache=v2)



이 튜토리얼에서는 간단히 전체 framebuffer에 그릴거라 scissor 사각형은 전체를 커버하도록 지정하겠습니다.

```cpp
VkRect2D scissor = {};
scissor.offset = { 0, 0 };
scissor.extent = swapChainExtent;
```

이제 이 viewport와 scissor 사각형은 **[VkPipelineViewportStateCreateInfo](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkPipelineViewportStateCreateInfo.html)** 구조체를 사용하여 viewport state에 결합되어야 합니다. 

몇몇 그랙픽 카드에서는 여러개의 viewport와 scissor 사각형을 사용하는 것이 가능합니다. 따라서 이 멤버는 그것들(viewport, scissor)의 배열을 참조합니다. 여러개를 사용하려면 GPU feature를 활성화(논리 디바이스 생성 챕터를 참고하세요)해야 합니다.

```cpp
VkPipelineViewportStateCreateInfo viewportState = {};
viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
viewportState.viewportCount = 1;
viewportState.pViewports = &viewport;
viewportState.scissorCount = 1;
viewportState.pScissors = &scissor;
```



# Rasterizer

rasterizer는 vertex shader의 vertex들로 형성한 geometry를 받아서 이를 fragment shader로 색칠 할 fragment로 변환합니다.

rasterzier는 또한 **[depth testing](https://en.wikipedia.org/wiki/Z-buffering)**, **[face culling](https://en.wikipedia.org/wiki/Back-face_culling)**, scissor test를 수행하고 전체 폴리곤이나 엣지만(wireframe rendering) 채울 fragment 출력을 구성할 수 있습니다. 

이 모든 것은 **[VkPipelineRasterizationStateCreateInfo](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkPipelineRasterizationStateCreateInfo.html)** 구조체를 사용하여 구성됩니다.

```cpp
VkPipelineRasterizationStateCreateInfo rasterizer = {};
rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
rasterizer.depthClampEnable = VK_FALSE;
```

**`depthClampEnable`** 이 **`VK_TRUE`** 로 설정되면 near/far plane를 벗어난 fragment들은 폐기되는 대신 **clamp** (near보다 작으면 near, far 보다 크면 far 리턴) 됩니다. 이 설정은 shadow map 같은 특별한 상황에서 유용합니다.

이 기능은 GPU freature 활성화가 필요합니다.

```cpp
rasterizer.rasterizerDiscardEnable = VK_FALSE;
```

**`rasterizerDiscardEnable`** 이 **`VK_TRUE`** 로 설정되면 geometry는 rasterizer 단계를 절대 진행하지 않습니다. 이 설정은 기본적으로 framebuffer에 어떤 output도 비활성화 합니다.

```cpp
rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
```

**`polygonMode`** 는 geometry를 위해 fragmnet가 생성되는 방법을 결정합니다. 아래의 모드들이 가능합니다.

- **`VK_POLYGON_MODE_FILL`** : polygon 영역을 fragment로 채웁니다.
- **`VK_POLYGON_MODE_LINE`** : polygon의 엣지를 선으로 그립니다.
- **`VK_POLYGON_MODE_POINT`** : polygon vertex를 점으로 그립니다.

fill을 제외한 다른 모드를 사용하려면 GPU feature 활성화가 필요합니다.

```cpp
rasterizer.lineWidth = 1.0f;
```

**`lineWidth`** 멤버는 직관적으로 fragment 수를 기준으로 선의 두께를 기술합니다. 지원되는 최대 선 두께는 하드웨어에 따라 달라지며, `1.0f`보다 두꺼운 선은 **`wideLines`** GPU feature의 활성화가 필요합니다.

```cpp
rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
```

**`cullMode`** 변수는 사용할 face culling의 유형을 결정합니다. culling을 비활성화 하거나, front face에서 cull을 수행하거나 back face에서 cull을 수행하거나 혹은 둘다(front/back culling)를 할 수 있습니다. **`frontFace`** 변수는 front-facing으로 간주될 face의 vertex 순서를 지정하며 clockwise나 counterclockwise가 될 수 있습니다.

```cpp
rasterizer.depthBiasEnable = VK_FALSE;
rasterizer.depthBiasConstantFactor = 0.0f; // Optional
rasterizer.depthBiasClamp = 0.0f; // Optional
rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
```

rasterzier는 상수값을 지정하거나 fragment 기울기를 기준으로 depth 값을 편향시켜서 depth 값을 변경할 수 있습니다. 이것은 때때로 shadow mapping에서 사용되지만, 우리는 이걸 사용하지 않을 것입니다. 간단히 **`depthBiasEnable`**를 **`VK_FALSE`**로 설정합니다.



# Multisampling

**[VkPipelineMultisampleStateCreateInfo](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkPipelineMultisampleStateCreateInfo.html)** 구조체는 multismapling을 구성합니다. 이는 **[anti-aliasing](https://en.wikipedia.org/wiki/Multisample_anti-aliasing)** 을 수행하기 위한 방법 중 하나입니다. 이것은 같은 pixel로 rasterize되는 여러 polygon의 fragment shader 결과를 결합하여 동작합니다. 이는 주로 엣지를 따라 발생하며, 엣지는 가장 눈에 띄는 aliasing artifact가 발생하는 곳이기도 합니다. 오직 하나의 polygon만 픽셀에 매핑되는 경우에는 fragment shader를 여러번 실행시킬 필요가 없기 때문에 단순히 더 높은 해상도로 렌더링 한 후 이를 다운스케일링하는 것보다 훨씬 저렴합니다. 이것(multisampling)을 활성화 하려면 GPU feature를 활성화해야 합니다.

```cpp
VkPipelineMultisampleStateCreateInfo multisampling = {};
multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
multisampling.sampleShadingEnable = VK_FALSE;
multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
multisampling.minSampleShading = 1.0f; // Optional
multisampling.pSampleMask = nullptr; // Optional
multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
multisampling.alphaToOneEnable = VK_FALSE; // Optional
```

나중에(multisampling 챕터) 이를 다시 볼거라 여기선 비활성화 상태를 유지하겠습니다.



# Depth and stencil testing

depth 와 sctencil buffer를 사용하려면 **[VkPipelineDepthStencilStateCreateInfo](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkPipelineDepthStencilStateCreateInfo.html)** 를 사용하여 depth, stencil test를 구성해야 합니다. 현재는 이것이 필요하지 않은지라, 이 구조체의 포인트를 넘기는 대신 간단히 **`nullptr`** 를 전달할 것입니다. 이것은 depth buffering 챕터에서 다시 확인하도록 하겠습니다.



# Color blending

fragment shader가 color를 반환한 이후, 이 color는 framebuffer에 이미 있는 color와 결합이 필요합니다. 이 변환을 color blending이라고 하며 이것을 위해서는 두가지 방법이 있습니다.

- old/new color를 혼합하여 최종 color를 만듭니다.
- old/new color를 bitwise operation을 이용하여 결합합니다.

color blending 구성을 위해서 두가지 유형의 구조체가 존재합니다. 첫번째 구조체는 **[VkPipelineColorBlendAttachmentState](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkPipelineColorBlendAttachmentState.html)** 로써 attach 된 framebuffer 마다의 구성을 포함합니다. 두번째 구조체는 **[VkPipelineColorBlendStateCreateInfo](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkPipelineColorBlendStateCreateInfo.html)** 로써 *global* blending 설정을 포함합니다. 우리는 하나의 framebuffer만 가지고 있습니다.

```cpp
VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

colorBlendAttachment.blendEnable = VK_FALSE;
colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
```

이 framebuffer당 구조체는 첫번째 color blending을 위한 방법을 구성할 수 있도록 해줍니다. 이 operation의 동작은 아래의 의사코드를 사용하여 잘 보여지고 있습니다.

```cpp
if (blendEnable) {
	finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb);

	finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
}
else {
	finalColor = newColor;
}

finalColor = finalColor & colorWriteMask;
```

**`blendEnable`** 이 **`VK_FLASE`** 로 설정되면 fragment shader의 새로운 color는 수정되지 않고 전달됩니다. 그렇지 않으면 새로운 color를 위해 두개의 혼합 operation이 동작합니다. 결과 color는 **`colorWriteMask`** 와 and 연산이 이루어져서 어떤 채널이 실제로 전달될지 결정하게 됩니다.

color blending을 위한 대표적인 방법은 alpha blending을 구현하는 것입니다. alpha blending은 새로운 color와 기존 color를 그들의 opacity를 기준으로 blending하는것입니다. **`finalColor`** 는 다음과 같이 계산됩니다.

```cpp
finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor;
finalColor.a = newAlpha.a;
```

이것은 아래의 파라미터들을 사용함으로써 얻을 수 있습니다.

```cpp
colorBlendAttachment.blendEnable = VK_TRUE;
colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
```

스펙문서에서 **[VkBlendFactor](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkBlendFactor.html)**와 **[VkBlendOp](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkBlendOp.html)** 열거값을 통해 가능한 모든 operation를 확인 할 수 있습니다.

두번째 구조체는 모든 framebuffer를 위한 구조체 배열을 참조하며 앞서 계산한 blend factor로 사용할 수 있는 blend constant를 설정할 수 있게 해줍니다.

```cpp
VkPipelineColorBlendStateCreateInfo colorBlending = {};
colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
colorBlending.logicOpEnable = VK_FALSE;
colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
colorBlending.attachmentCount = 1;
colorBlending.pAttachments = &colorBlendAttachment;
colorBlending.blendConstants[0] = 0.0f; // Optional
colorBlending.blendConstants[1] = 0.0f; // Optional
colorBlending.blendConstants[2] = 0.0f; // Optional
colorBlending.blendConstants[3] = 0.0f; // Optional
```

만약 blending의 두번째 방법(bitwise combination)을 사용하고 싶다면, **`logicOpEnable`**을 **`VK_TRUE`** 로 설정해야 합니다. bitwise operation은 \**`logicOp`\** 필드를 통해 정의될 수 있습니다. 이렇게 하면(**`logicOpEnable`** 을 **`VK_TURE`** 로 설정하면) 모든 attached framebuffer에 대해 **`blendEnable`** 을 **`VK_FALSE`** 로 설정한 것 처럼 첫번째 방법이 자동적으로 비활성화 됩니다. **`colorWriteMask`** 는 이 모드에서도 실제 영향을 받을 framebuffer의 채널을 결정하는데 사용됩니다. 두 모드를 모두 비활성화 하는 것도 가능합니다. 우리는 여기서 그렇게했죠. 이렇게 되면 fragment color는 수정없이 framebuffer에 쓰여집니다.



# Dynamic state

이전에 지정한 구조체 중 제한된 몇개의 상태는 실제로 pipeline 재생성 없이 변경할 수 있습니다. viewport, line width, blend constant를 예로 들 수 있습니다. 이렇게 하길 원한다면 **[VkPipelineDynamicStateCreateInfo](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkPipelineDynamicStateCreateInfo.html)** 구조체를 아래처럼 채워야 합니다.

```cpp
VkDynamicState dynamicStates[] = {
	VK_DYNAMIC_STATE_VIEWPORT,
	VK_DYNAMIC_STATE_LINE_WIDTH
};

VkPipelineDynamicStateCreateInfo dynamicState = {};
dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
dynamicState.dynamicStateCount = 2;
dynamicState.pDynamicStates = dynamicStates;
```

이 설정으로 인해 이들 값의 구성을 무시되고 드로잉 시점에 해당 데이터를 지정하는것이 필요하게 됩니다. 우리는 나중 챕터에서 이것을 다시 할겁니다. 이 구조체는 나중에 더이상 동적인 상태가 없을 때 **`nullptr`** 로 대체 될 수 있습니다.



# Pipeline layout

우리는 shader 안에서 **`uniform`** 값을 사용할 수 있습니다. 이것은 global 값으로 dynamic state 변수와 유사하게 shader의 동작을 shader 재생성 없이 drawing 시점에 바꿀수 있습니다. 이것은 대개 vertex shader에 transformation matrix를 전달하거나 fragment shader에서 texture sampler를 생성하기 위해서 사용됩니다.

이 uniform 값은 **[VkPipelineLayout](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkPipelineLayout.html)** 오브젝트 생성을 통해 pipeline을 생성하는 동안 지정되어야 합니다. 앞으로 몇 챕터 동안 이것을 사용하지 않을 것이지만, 그럼에도 불구하고 빈 pipeline layout을 생성해야 합니다.

이 값을 저장할 클래스 멤버를 생성합니다. 나중에 다른 함수에서 이를 참조할 것이기 때문입니다.

```cpp
VkPipelineLayout pipelineLayout;
```

그리고 **`createGraphicPipeline`** 함수 내에서 이 오브젝트를 생성합니다.

```cpp
VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
pipelineLayoutInfo.setLayoutCount = 0; // Optional
pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
	throw std::runtime_error("failed to create pipeline layout!");
}
```

이 구조체는 shader에 전달할 수 있는 또하나의 동적 값인 ***push constant***를 지정하는데 이는 이후 챕터에서 사용하게 될것입니다. pipeline layout은 프로그램 라이프사이클 내내 참조될 것입니다. 때문에 프로그램 종료 시점에 이를 폐기해야 합니다.

```cpp
void cleanup() {
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	...
}
```



# Conclusion

이게 모든 fixed-function 상태에 대한 것입니다. 처음부터 이것을 모두 설정하는것은 많은 작업이긴 하지만, 이제 우리는 graphics pipeline에서 진행되는 모든 작업을 거의 완전히 이해했습니다. 이를 통해 특정 컴포넌트의 기본 상태가 우리가 우리가 원한 것과 다르거나 하는 이유로 예기치 않은 동작을 발생기킬 가능성을 줄여줍니다.

하지만 최종적으로 graphics pipeline를 생성시키기 위해서는 생성해야 할 오브젝트가 하나 더 있습니다. 바로 [render pass](https://www.notion.so/Drawing-a-triangle-Graphics-pipeline-basics-Render-passes-6c9c59df3ee845e6a94aafd1e4e86f9e)입니다.

**[C++ code](https://vulkan-tutorial.com/code/10_fixed_functions.cpp)** / **[Vertex shader](https://vulkan-tutorial.com/code/09_shader_base.vert)** / **[Fragment shader](https://vulkan-tutorial.com/code/09_shader_base.frag)**

