우리는 이전 챕터의 모든 구조체와 오브젝트를 조합하여 이제 graphics pipeline를 생성할 수 있습니다!

여기 지금 우리가 가진 오브젝트의 유형이 있습니다.

- **Shader stages** : Shader 모듈은 graphics pipeline의 프로그래밍 가능 단계의 기능을 정의합니다.
- **Fixed-function state** : 모든 구조체는 input assembly, rasterizer, viewport, color blending 같은 pipeline의 Fixed-function 단계를 정의합니다.
- **Pipeline layout** : shader에 의해 참조되는 uniform과 push 변수는 draw time에 업데이트 될 수 있습니다.
- **Render pass** : pipeline 단계에서 참조하는 attachment와 그 사용법

이 모든 것들이 조합되어 graphics pipeline의 기능을 완전하게 정의합니다. 그래서 우리는 이제 **`createGraphicsPipeline`** 함수의 마지막에 **[VkGraphicsPipelineCreateInfo](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkGraphicsPipelineCreateInfo.html)** 구조체를 채울 수 있습니다. 하지만 **[vkDestroyShaderModule](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkDestroyShaderModule.html)** 호출 전에 해야 하는데 그 이유는 shaderModule이 pipeline을 생성하는 동안에도 사용되기 때문입니다.

```cpp
VkGraphicsPipelineCreateInfo pipelineInfo = {};
pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
pipelineInfo.stageCount = 2;
pipelineInfo.pStages = shaderStages;
```

**[VkPipelineShaderStageCreateInfo](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkPipelineShaderStageCreateInfo.html)** 구조체 배열을 참조하는 것 부터 시작합니다.

```cpp
pipelineInfo.pVertexInputState = &vertexInputInfo;
pipelineInfo.pInputAssemblyState = &inputAssembly;
pipelineInfo.pViewportState = &viewportState;
pipelineInfo.pRasterizationState = &rasterizer;
pipelineInfo.pMultisampleState = &multisampling;
pipelineInfo.pDepthStencilState = nullptr; // Optional
pipelineInfo.pColorBlendState = &colorBlending;
pipelineInfo.pDynamicState = nullptr; // Optional
```

그리고나서 Fixed-function 단계를 기술한 모든 구조체를 참조합니다.

```cpp
pipelineInfo.layout = pipelineLayout;
```

그 이후 pipeline layout이 옵니다. 여기서는 구조체의 포인터 대신 Vulkan 핸들을 사용합니다.

```cpp
pipelineInfo.renderPass = renderPass;
pipelineInfo.subpass = 0;
```

그리고 마지막으로 이 graphics pipeline에서 사용할 render pass와 subpass 인덱스를 참조합니다. 이 특정 인스턴스 대신 다른 reader pass를 pipeline에 사용 할 수도 있지만, **`renderPass`**와 **호환**되어야 합니다. 이 호환성 요구사항은 **[여기](https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#renderpass-compatibility)**에 기술되어 있습니다. 허나 이 feature는 튜토리얼에서는 사용하지 않을 것입니다.

```cpp
pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
pipelineInfo.basePipelineIndex = -1; // Optional
```

실제로 두개의 파라미터가 더 있습니다: **`basePipelineHandle`** 과 **`basePipelineIndex`**. Vulkan은 기존 pipeline을 상속받아서 새로운 graphics pipeline를 생성할수 있도록 해줍니다. 이 pipeline 상속 개념은 상속 pipeline이 기존 pipeline과 공통된 많은 기능을 가지고 있을때  pipeline 설정에 더 적은 비용이 들고 같은 부모를 가진 pipeline들 간의 스위칭 또한 빠릅니다.  기존 pipeline을 **`basePipelineHandle`**에 지정하거나 **`basePipelineIndex`**를 인덱스로 생성하려는 다른 pipeline을 참조 할 수 있습니다. 현재 오직 하나의 pipeline만 있기 때문에 간단히 null 핸들과 invalid 인덱스를 지정합니다. 이 값들은 **[VkGraphicsPipelineCreateInfo](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkGraphicsPipelineCreateInfo.html)**의 **flag**필드에 **`VK_PIPELINE_CREATE_DERIVATIVE_BIT`**를 사용할 때만 가능합니다.

마지막 단계를 준비하기 위해 **[VkPipeline](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkPipeline.html)** 오브젝트를 저장 할 클래스 멤버를 생성합니다.

```cpp
VkPipeline graphicsPipeline;
```

그리고 최종적으로 graphics pipeline를 생성합니다.

```cpp
if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
	throw std::runtime_error("failed to create graphics pipeline!");
}
```

**[vkCreateGraphicsPipelines](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCreateGraphicsPipelines.html)** 함수는 Vulkan에서 다른 오브젝트를 생성하는 함수보다 실제로 더 많은 파라미터를 가집니다. 이는 여러개의 **[VkGraphicsPipelineCreateInfo](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkGraphicsPipelineCreateInfo.html)**를 받도록 설계되었고 단일 호출로 여러개의 **[VkPipeline](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkPipeline.html)** 오브젝트를 생성합니다.

두번째 파라미터를 위해 **`VK_NULL_HANDLE`** 인자를 전달합니다. 이 파라미터는 선택적으로 **[VkPipelineCache](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkPipelineCache.html)** 오브젝트를 참조합니다. pipeline 캐시는 **[vkCreateGraphicsPipelines](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCreateGraphicsPipelines.html)**에 대한 여러 호출에서 pipeline 생성과 관련된 데이터를 저장하고 재사용하는데 사용할 수 있고 캐시가 파일에 저장되어 있는 경우 프로그램 실행 전반에서 사용될 수 있습니다. 따라서 이후 pipeline 생성 속도를 크게 높일 수 있습니다. 우리는 pipeline cache 챕터에서 이를 다룰 것입니다.

graphics pipeline은 일반적인 모든 drawing operation에서 필요한지라 프로그램의 끝에서 폐기되어야 합니다.

```cpp
void cleanup() {
	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	...
}
```

이제 이 모든 노력을 통해 성공적으로 pipeline이 생성되는 것을 확인하기 위해 프로그램을 실행하십시오! 우리는 이미 화면상에 무엇인가를 띄워서 볼수 있게 되는 것에 매우 가까워졌습니다. 다음 두서너 챕터에서 swap chain images로 부터 실제로 framebuffer를 설정하고 drawing commnad를 준비할 것입니다.

**[C++ code](https://vulkan-tutorial.com/code/12_graphics_pipeline_complete.cpp)** / **[Vertex shader](https://vulkan-tutorial.com/code/09_shader_base.vert)** / **[Fragment shader](https://vulkan-tutorial.com/code/09_shader_base.frag)**
