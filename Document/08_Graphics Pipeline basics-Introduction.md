다음 몇 강좌를 진행하는 동안 우리의 첫 삼각형을 그리기위해 구성되는 graphics pipeline을 세팅할 것입니다. graphics pipeline은 render target의 픽셀까지 메쉬의 버텍스와 텍스쳐를 온전히 이동시키는 일련의 operation입니다. 아래에 간단한 개요가 표시됩니다.

![vulkan_simplified_pipeline.svg](https://s3-us-west-2.amazonaws.com/secure.notion-static.com/ecd1eb1e-7b15-4e4e-aa7b-f989e0552867/vulkan_simplified_pipeline.svg)

![이미지 653.png](https://s3-us-west-2.amazonaws.com/secure.notion-static.com/a898f555-bcce-424f-9bf3-b76b55c0487b/이미지_653.png)

***input assembler***는 지정된 버퍼에서 원시(raw) vetex 데이터를 수집하며, vertex 데이터 자체가 중복을 가지지 않게  index 버퍼를 사용하여 특정 요소를 반복할 수도 있습니다.

***vertex shader***는 모든 vertex에 대해 실행되며 일반적으로 model 공간에서 screen 공간으로 vertex 좌표를 전환하기 위해 transformation을 적용합니다. 또한 vertex 데이터 각각을 다음 pipeline으로 전달합니다.

***tessellation shader***는 메쉬 퀄리티를 올리기 위해 특별한 룰을 베이스로 geometry를 세부화하게 해줍니다. 이것은 때때로 가까이에 있는 벽돌 벽이나 계단 같은 것이 덜 평평해 보이도록 사용됩니다.

***geometry shader***는 모든 primitive (triangle, line, point)에 대해 실행되고 이를 버리거나 들어온 것보다 더 많은 primitive를 출력할 수 있습니다. 이것은 tessellation sahder와 유사하지만 훨씬 더 유연합니다. 하지만 intel의 통합 GPU를 제외하고 대부분의 그래픽 카드에서 좋지 않은 성능을 보이기 때문에 오늘날 어플리케이션에서 많이 사용되고 있지는 않습니다. ( 🐸 제가 알기로는 Metal에는 이 단계가 없어요.)

***rasterization*** 단계는 primitive를 fragment로 분해합니다. 이들(frament)은 픽셀 요소로서, framebuff를 채웁니다. 화면 밖의 모든 fragment는 폐기됩니다. 그리고 위의 그림에서 처럼 vertex shader가 출력한 attribute가 fragment들에 보간됩니다. 다른 primitive fragment뒤에 있는 fragment들도 depth testing으로 인해 대개 여기서 폐기됩니다.

***fragment shader***는 살아남은 모든 fragment로 부터 불리워지고 어떤 framebuffer(혹은 framebuffer들)에 쓰여질지, 어떤 color 혹은 어떤 depth 값을 사용할지 결정합니다. vertex shader로부터의 보간된 데이터를 사용하여 이를 수행할 수 있는데 여기에는 textrue 좌표나 라이팅 노말 같은 값도 들어갈 수 있습니다.

***Color blending*** 단계는 framebuffer안에 동일한 픽셀로 매팅되는 여러 fragment를 혼합하는 operation을 적용합니다. fragment들은 단순히 서로 덮어쓰거나 투명도에 따라 합산되거나 혼합될 수 있습니다.

녹색 단계는 ***fixed-function*** 단계라고 알려져 있습니다. 이 단계는 파라미터를 사용하여 그들의 operation을 수정할 수 있게 해주지만, 작동하는 방법 자체는 미리 정의되어 있습니다.

반면에 주황색 단계는 **`programmable`**합니다. 이 말은 여러분의 코드를 그래픽 카드에 업로드해서 정확히 여러분이 원하는 operation을 수행할 수 있다는 의미입니다. 예를들어 fragment shader를 사용하여 텍스쳐링과 라이팅에서부터 레이 트레이서까지 모든 것을 구현할 수 있습니다. 이 프로그램은 많은 GPU 코어에 동시에 실행되어 여러 오브젝트(vertex나 fragment 같은)를 병렬로 처리합니다.

여러분이 이전에 OpenGL이나 Direct3D 같은 예전 API를 사용했었다면 **`glBendFunc`**이나 **`OMSetBlendState`**같은 호출을 사용하여 마음대로 어떤 pipeline 설정도 변경할 수 있었을 겁니다.  Vulkan의 graphics pipeline은 거의 완전히 변경할 수 없습니다. 때문에 여러분이 shader를 변경하고 싶다거나, 다른 framebuffer에 바인딩을 하고 싶다거나 , blend 함수를 바꾸고 싶을때면 처음부터 pipeline를 다시생성해야 합니다. 이 방식의 단점은 여러분의 rendering operation에서 여러분이 사용하고 싶은 모든 상태 조합을 나타내는 여러 pipeline를 생성해야 한다는 것입니다. 그러나, 여러분이 pipeline을 통해 수행할 모든 operation은 사전에 알려지기 때문에 드라이버를 이를 훨씬 더 효과적으로 최적화 할 수 있습니다.

몇가지 프로그래머블 단계는 여러분이 무엇을 할지에 따라서 선택적입니다. 예를 들어 tessellation이나 geometry 단계는 단지 간단한 geometry만 그릴거라면 비활성화 할 수 있습니다. 만약 오직 depth 값에만 관심이 있다면 fragment shader 단계를 비활성화 할 수 있습니다. 이건 **[shadow map](https://en.wikipedia.org/wiki/Shadow_mapping)**를 생성할 때 유용합니다.

다음 챕터에서는 첫번째로 삼각형을 화면에 넣는데 필요한 두개의 프로그래머블 단계(vertex shader, fragment shader)를 생성하겠습니다. blend mode, viewport, rasterization 같은 fixed-function의 구성은 그 이후 장에서 설정 할 것입니다. Vulkan의 graphics pipeline을 설정하는 마지막 부분은 입력/출력 frambuffer와 관련됩니다.

**`createGraphicsPipeline`** 함수를 생성하고, 이를 **`initVulkan`**안의 **`createImageViews`** 바로 뒤에서 호출합니다. 다음 챕터 내내 이 함수에서 작업할 겁니다.

```cpp
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createGraphicsPipeline();
}

...

void createGraphicsPipeline() {
}
```

**[C++ code](https://vulkan-tutorial.com/code/08_graphics_pipeline.cpp)**
