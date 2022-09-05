# Introduction
다음 몇 챕터에서 우리는 vertex shader의 하드코딩된 vertex data를 memory의 vertex buffer로 교체할 것입니다.
우리는 CPU 가시적 buffer를 생성하기 위해 `memcpy`를 이용해 vertex data를 바로 카피하는  가장 쉬운 접근부터 시작해서 이후에 staging buffer를 이용해 vertex data를 고성능 memory에 복사하는 방법을 볼 것입니다.

## Vertex Shader
먼저 vertex shader를 변경하여 더 이상 shader code 자체에 vertex data를 포함하지 않습니다.
`in` 키워드를 사용하면 vertex shader는 vertex buffer에서 입력을 받습니다.

```
#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}
```

`inPosition`와 `inColor`는 vertex attribute입니다. 두 배열을 사용하여 vertex당 position과 color를 수동으로 지정한 것 처럼 vertex buffer에 vertex 별로 지정되는 attribute입니다.
vertex shader를 다시 컴파일 하십시시오. 
`fagColor`와 마찬가지로 layout(location = x)은 나중에 참조하는데 사용할 수 있도록 input index를 할당합니다. dvec3 64bit vector와 같은 멀티 슬롯을 쓰는 일부 유형에 중요합니다.  
이는 그 뒤의 index가 최소 2이상 높아야 함을 의미합니다.

```
layout(location = 0) in dvec3 inPosition;
layout(location = 2) in vec3 inColor;
```

당신은 [OpenGL wiki](https://www.khronos.org/opengl/wiki/Layout_Qualifier_(GLSL), "OpenGL wiki")에서 layout 한정자에 대한 정보를 더 알수 있습니다.

## Vertex data
 vertex data를 shader code에서 우리 code의 program안의 배열로 이동합니다. vector 및 matrix같은 선형 대수 관련 유형을 제공하는 GLM library를 추가하고 시작하십시오.
 우리는 이것을 통해 position과 color vector 지정합니다.
 
 ```
 #include <glm/glm.hpp>
 ```
 
 `Vertex`라는 이름을 가진 두 속성을 가진 struct를 생성하십시오. 우리는 이것을 vertex shader 안에 공유할 것입니다.  
 
 ```
 struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
}; 
 ```
 
 이제 vertex structure를 사용하여 vertex data의 배열에 지정합니다. 우리는 정확이 전과 같은 position과 color 값을 사용하고 있지만 지금은 그들이 vertex의 배열 안에서 결합됩니다. 이것을 vertex attribute _interleaving_이라고 합니다.
 
 
 ## Binding descriptions
 다음 스탭은 GPU memory에 업로드한 이후 data format을 vertex shader에 전달하는 방법을 Vulkan에 알리는 것입니다. 정보를 전달하는데 필요한 struct에는 두 가지 유형이 있습니다.
  첫 번째 구조는 `VkVertexInputBindingDescription`이며 올바른 데이터로 채우기 위해 Vertex 구조에 멤버 변수를 추가합니다.
  ```
  struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};

        return bindingDescription;
    }
};
```

vertex binding은 vertex 전체에 걸쳐 메모리에서 데이터를 로드하는 rate를 설명합니다. 데이터 항목 사이의 byte 수와 각 vertex 또는 instance 이후에 다음 데이터 항목으로 이동할지 여부를 지정합니다.

```
VkVertexInputBindingDescription bindingDescription{};
bindingDescription.binding = 0;
bindingDescription.stride = sizeof(Vertex);
bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
```

모든 vertex별 데이터는 하나의 배열에 함께 포장되므로 하나의 binding만 갖게 됩니다. binding 매개변수는 binding 배열에서 binding index를 지정합니다.  
stride 매개변수는 한 항목에서 다음 항목까지의 byte 수를 지정하고 inputRate 매개변수는 다음 값 중 하나를 가질 수 있습니다.  

- VK_VERTEX_INPUT_RATE_VERTEX : vertex 뒤의 다음 데이터 항목으로 이동
- VK_VERTEX_INPUT_RATE_INSTANCE : 각 instance 후 다음 데이터 항목으로 이동

instance 렌더링을 사용하지 않을 것이므로 vertex data를 사용하겠습니다.

  
## Attribute descriptions

vertex input을 처리하는 방법을 설명하는 두 번째 구조는 `VkVertexInputAttributeDescription`입니다.  
이 struct를 채우기 위해 Vertex에 다른 헬퍼 함수를 추가할 것입니다.
  
```
#include <array>

...

static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

    return attributeDescriptions;
}
```

함수 프로토타입에서 알 수 있듯이 이러한 구조는 두 가지가 될 것입니다. attribute description struct는 binding description에서 비롯된 vertex data 청크에서 vertex attribute를 추출하는 방법을 설명합니다.  
position과 color라는 두 개의 속성이 있으므로 두 개의 attribute description struct가 필요합니다.


```
attributeDescriptions[0].binding = 0;
attributeDescriptions[0].location = 0;
attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
attributeDescriptions[0].offset = offsetof(Vertex, pos);
```

`binding` 매개변수는 Vulkan에 vertex별 data의 binding을 알려줍니다. location 매개변수는 vertex shader에서 input의 location 지시문을 참조합니다.  
`location`이 0인 vertex shader의 입력은 두 개의 32bit float component가 있는 position입니다.
`format` 매개변수는 속성에 대한 data format을 설명합니다. 약간 혼란스럽게도 형식은 color format과 동일한 enum을 사용하여 지정됩니다. shader type 및 format은 일반적으로 다음과 같이 사용됩니다.

- float: VK_FORMAT_R32_SFLOAT
- vec2: VK_FORMAT_R32G32_SFLOAT
- vec3: VK_FORMAT_R32G32B32_SFLOAT
- vec4: VK_FORMAT_R32G32B32A32_SFLOAT

보다싶이 color channel의 양이 shader data 유형의 component 수와 일치하는 형식을 사용해야합니다. shader의 component수 보다 더 많은 channel을 사용할 수 있지만 자동으로 삭제됩니다. channel수가 component 수보다 적으면 BGA component는 기본값(0, 0, 1)을 사용합니다.
color 유형(SFLOAT, UINT, SINT)및 bit width도 shader input 유형과 일치해야 합니다. 
다음 예를 참고하십시오.  

- ivec2: VK_FORMAT_R32G32_SINT, a 2-component vector of 32-bit signed integers
- uvec4: VK_FORMAT_R32G32B32A32_UINT, a 4-component vector of 32-bit unsigned integers
- double: VK_FORMAT_R64_SFLOAT, a double-precision (64-bit) float

`format` 매개변수는 attribute data의 byte 크기를 암시적으로 정의하고 offset 매개변수는 읽은 vertex별 데이터의 시작 이후 byte수를 지정합니다. 
binding은 한 번에 하나의 vertex를 로드하고 position attribute(`pos`)는 이 struct의 시작 부분에서 `0` byte offset에 있습니다. 이것은 `offsetof` 매크로를 사용하여 자동으로 계산됩니다.

## Pipeline vertex input

이제 createGrahpincsPipeline의 struct를 참조하여 이 형식의 vertex data를 허용하도록 graphic pipeline을 설정해야 합니다.
vertexinputinfo struct를 찾아 두 description을 참조하도록 수정합니다.

```
auto bindingDescription = Vertex::getBindingDescription();
auto attributeDescriptions = Vertex::getAttributeDescriptions();

vertexInputInfo.vertexBindingDescriptionCount = 1;
vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
```

pipeline은 이제 vertex 컨테이너 형식의 vertex data를 받아 vertex shader에 전달할 준비가 되었습니다.  
validation layer가 활성화 된 상태에서 program을 실행하면 binding에 binding된 vertex buffer가 없다고 말하는 것을 볼 수 있습니다.  
다음 단계는 vertex buffer를 만들고 vertex data를 GPU가 액세스 할 수 있도록 vertex buffer로 이동하는 것입니다.  

[C++ code / Vertex shader / Fragment shader](https://vulkan-tutorial.com/code/18_shader_vertexbuffer.frag)







