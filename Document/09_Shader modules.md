

이전 API들과는 달리, Vulkan의 shader code는 **[GLSL](https://en.wikipedia.org/wiki/OpenGL_Shading_Language)**이나 **[HLSL](https://en.wikipedia.org/wiki/High-Level_Shading_Language)** 처럼 사람이 읽을 수 있는 구문이 아닌 bytecode 포맷으로 지정되어야 합니다. 이 bytecode 포맷을 **[SPIR-V](https://www.khronos.org/spir)**라고 하며 Vulkan과 OpenCL(둘다 Khronos의 API임)에서 사용하기 위해 설계되었습니다. 이것은 graphics과 compute shader를 작성하기 위해 사용할 수 있는 포맷입니다. 허나 이 튜토리얼에서는 Vulkan의 graphics pipeline에서 사용하는 shader에 초첨을 맞출 것입니다.

Bytecode 포맷 사용의 이점은 GPU 벤더가 shader code를 native code로 바꾸기위한 작성한 컴파일러가 상당히 덜 복잡하다는 점입니다. 과거 GLSL같은 사람이 읽을 수 있는 문법을 사용하여 일부 GPU 벤더들은 표준을 해석하는데 다소 유연했었습니다. 이들 벤더 중 하나의 GPU로 중요한 shader를 작성한다면, 다른 벤더의 드라이버에서 문법 오류나 경고로 인해 해당 코드를 거부하거나 컴파일러 버그로 인해 shader가 다르게 동작할 위험이 있습니다. SPIR-V같은 간단한 bytecode 형식을 사용하면 이를 피할수 있을 것입니다.

하지만, 이것이 bytecode를 직접 작성해야 하다는 의미는 아닙니다. Khronos는 GLSL을 SPIR-V로 컴파일하는 벤더 독립적인 자체 컴파일러를 릴리즈 했습니다. 이 컴파일러는 사용자의 shader 코드가 표준을 완벽하게 준수하는지 검증하고 어플리케이션에 적재할 수 있는 하나의 SPIR-V 바이너리를 생성하도록 설계되었습니다. 이 컴파일러를 라이브러리 형태로 포함시켜 런타임에 SPIR-V를 만들 수도 있지만, 이 튜토리얼에서는 그렇게 사용하지 않을 것입니다. `glslangValidator.exe`를 통해 이 컴파일러를 직접 사용할 수도 있지만 우리는 대신 Google의 `glslc.exe`를 사용할 것입니다.  `glslc`의 장점은 GCC나 Clang과 같이 잘 알려진 컴팡일러와 동일한 파라미터 형식을 사용하고, include같은 추가적인 기능을 포함입니다. 이들 모두 이미 Vulkan SDK에 포함되어 있으므로 이를 다운로드하기 위한 어떤 추가작업도 필요없습니다.

GLSL은 C 스타일 문법을 가진 shading 언어입니다. 작성된 프로그램은 **`main`** 함수를 가지고 있고 이 함수는 모든 오브젝트에 대해 호출됩니다. input을 위한 파라미터나 output을 위한 리턴 값 대신 GLSL은 input과 output을 제어하기 위해 글로번 변수를 사용합니다. GLSL은 built-in vector, matrix primitive 같이 그래픽스 프로그래밍을 돕기 위해 여러가지 기능을 포함하고 있습니다. cross product, matrix-vector product, vector 주위의 반사 같은 작업을 위한 함수도 포함되어 있습니다. vector 타입은 **`vec`** 와 요소의 갯수를 나타내는 숫자로 이루어집니다. 예를들어 3D 위치는 **`vec3`**에 저장됩니다. **`.x`** 같은 멤버를 통해 단일 컴포넌트에 엑세스 할 수 있고, 동시에 여러 컴포넌트에서 새로운 vector를 생성할 수도 있습니다. 예를들어 표현식 **`vec3(1.0, 2.0, 3.0).xy`**은 **`vec2`**가 됩니다. vector 생성자는 vector 오브젝트와 스칼라 값의 조합을 취할 수도 있습니다. 예를들어 **`vec3(vec2(1.0, 2.0), 3.0)`**으로 **`vec3`**를 생성할수 있습니다.

이전 챕터에서 언급했듯이 화면위에 삼각형을 배치시키기 위해서 vertex shader와 fragment shader를 작성해야 합니다. 다음 두 섹션에서 각각의 GLSL 코드를 살펴보겠습니다. 그 후 두개의 SPIR-V 바이너리를 어떻게 생성하는지, 그것들을 프로그램에서 어떻게 로드하는지를 보여드리겠습니다.



# Vertex shader

Vertex shader는 들어오는 각각의 vertex를 처리합니다. Vertex shader는 vertex의 **word position, color, normal, texture coordinate** 같은 속성을 `input`으로 사용합니다. 

`output`은 clip 좌표의 마지막 위치와 fragment shader로 넘겨야 할 필요가 있는 속성들(예 : color, texture coordinate)입니다. 

그런 다음 이 값들은 rasterizer에 의해 fragment 위에 보간되어 부드러운 그라디언트가 됩니다.

***clip coordinate***는 vertex shader의 4차원 vector이며 전체 vector를 그들의 마지막 컴포넌트로 나눠서 정규화된 디바이스 좌표(***normalized device coordinate***)로 변환된 결과입니다. 이 정규화된 디바이스 좌표는 아래처럼 frambuffer를 [-1, 1] 에서 [1,-1]의 좌표계로 매핑하는 **[동차좌표(homogeneous coordinate)](https://en.wikipedia.org/wiki/Homogeneous_coordinates)**입니다.



![img](https://dolong.notion.site/image/https%3A%2F%2Fs3-us-west-2.amazonaws.com%2Fsecure.notion-static.com%2F05b06f6c-cf1b-49dc-be33-ddca7849eef0%2Fnormalized_device_coordinates.svg?table=block&id=8674fb1c-630e-4797-a9d7-f45e22f59465&spaceId=09757d3e-a434-430f-8919-d37a4c3bb1a4&userId=&cache=v2)



이전에 컴퓨터 그래픽을 조금이라도 했다면 이미 이것에 익숙해져 있을 겁니다. 이전에 OpenGL를 사용했었다면 Y 좌표의 부호가 뒤집혀 있음을 알 수 있습니다. Z 좌표는 Direct3D와 같이 `0 부터 1`까지의 범위를 가집니다.

우리의 첫번째 삼각형을 위해서 어떤 transformation도 적용하지 않을 겁니다. 우리는 그저 3개의 vertex 위치를 정규화된 디바이스 좌표로 직접 지정하여 아래의 모양을 생성할 것입니다.



![img](https://dolong.notion.site/image/https%3A%2F%2Fs3-us-west-2.amazonaws.com%2Fsecure.notion-static.com%2F9c514d17-65ae-447b-987d-cc3da2a19fa6%2Ftriangle_coordinates.svg?table=block&id=050b0102-25f7-4b7f-9a77-44fc6ad2109b&spaceId=09757d3e-a434-430f-8919-d37a4c3bb1a4&userId=&cache=v2)



우리는 vertex의 마지막 요소(w)를 **`1`**로 지정한 vertex shader로 부터 clip 좌표를 출력하여 정규화된 디바이스 좌표를 직접 출력할 수 있습니다. 

이렇게 하면 clip 좌표를 정규화된 디바이스 좌표로 transform 하는 부분에서 아무것도 바뀌지 않습니다.

일반적으로 이 좌표는 vertex buffer에 저장되지만, vertex buffer를 생성하고 데이터로 그것을 채우는 것은 쉽지 않은 작업입니다. 그래서 저는 우리가 삼각형을 화면에 띄우는걸 보고 만족할 때 까지 이 작업을 미루겠습니다. 

그동안 우리는 약간 전통적이지 않은 것을 할겁니다: vertex shader안에 좌표를 포함시키는 거죠. 

코드는 아래와 같습니다.

```glsl
#version 450

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}
```

**`main`** 함수는 모든 vertex를 위해 불리워 집니다. Built-in **`gl_VertexIndex`** 변수는 현재 vertex의 index를 가지고 있습니다. 

이것은 일반적으로 vertex buffer안의 인덱스를 의미하지만, 여기서는 하드코딩된 vertex 데이터 배열의 인덱스입니다. 

각 vertex의 위치는 shader안의 상수 배열로부터 엑세스 되고 더미 **`z`** 와 **`w`** 컴포넌트가 결합되어 clip 좌표에서의 위치를 만듭니다. 

Built-in 변수인 **`gl_Position`**은 output으로 사용됩니다.



# Fragment shader

Vertex shader의 position에 의한 형성된 삼각형은 화면 영역을 fragment로 채웁니다.

 fragment shader는 이 fragment들에서 호출되며 frambuffer(s)를 위해 `color`나 `depth`를 생성합니다. 

삼각형 전체를 붉은색으로 output하는 간단한 fragment shader는 아래와 같습니다.

```glsl
#version 450

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}
```

Vertex shader의 **main** 함수가 모든 vertex에서 호출되는 것 처럼 이 **`main`** 함수는 모든 fragment에서 호출됩니다. 

GLSL에서 Color는 [0,1] 범위의 `R,G,B,A`의 4개의 채널이 있는 4-컴포넌트 vector입니다. 

Vertex Shader의 **`gl_Position`**과는 다르게, 여기에는 현재 fragment의 color output을 위한 built-in 변수가 존재하지 않습니다. **`layout(location = 0)`** 수정자가 지정한 인덱스의 framebuffer 각각을 위한 고유한 output 변수를 지정해야 합니다. 

붉은 색은 인덱스 **`0`**의 첫번째(현재 유일한) framebuffer에 링크된 **`outColor`** 변수에 쓰여집니다.



# Per-vertex colors

전체가 빨간 삼각형을 만드는건 그다지 재밌지 않습니다. 다음과 같은 것이 훨씬 나아보이지 않을까요?



![img](https://dolong.notion.site/image/https%3A%2F%2Fs3-us-west-2.amazonaws.com%2Fsecure.notion-static.com%2F5d96e215-cba3-4383-8ce3-d4334b983a0e%2Ftriangle_coordinates_colors.png?table=block&id=b61e60e0-1e2c-46fa-a353-1ba5bf46620b&spaceId=09757d3e-a434-430f-8919-d37a4c3bb1a4&width=550&userId=&cache=v2)



이것을 이루기 위해서는 두 shader에 몇가지 수정을 해야 합니다. 먼저 3개의 vertex 각각에 개별 color를 지정해야 합니다. 

Vertex shader에는 이제 position 처럼 color 배열을 추가해야 합니다.



```glsl
vec3 colors[3] = vec3[](
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 1.0)
);
```

이제 이 vertex당 color를 fragment shader에 전달해야 합니다. 그러면 fragment shader는 그들(vertex 당 color)를 보간한 값을 framebuffer에 output할 수 있습니다. Color를 위한 output을 vertex shader에 추가하고 이를 **`main`** 함수에 씁니다.

```glsl
layout(location = 0) out vec3 fragColor;

void main() {
	gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
	fragColor = colors[gl_VertexIndex];
}
```

이제 fragment shader에 매칭되는 input를 추가합니다.

```glsl
layout(location = 0) in vec3 fragColor;

void main() {
	outColor = vec4(fragColor, 1.0);
}
```

Input 변수가 같은 이름일 필요는 없습니다. 그들은 **`location`**의 지시에 의한 **인덱스** 지정을 사용하여 서로 링크됩니다. 

**`main`** 함수는 color 와 함께 alpha를 output 하도록 수정되었습니다. 위의 그림에서 보듯이 **`fragColor`**를 위한 값은 3개의 vertex 사이의 fragment들을 위해 자동적으로 보간되어서 부드러운 그라디언트 결과가 됩니다.



# Compiling the shader

프로젝트의 root 디렉토리에 **`shaders`** 디렉토리를 생성합니다. 해당 디렉토리에 **`shader.vert`** 파일에 vertex shader를 저장하고 **`shader.frag`** 파일에 fragment shader를 저장합니다. 

GLSL shader에는 공식적인 확장자가 존재하지 않지만, 이 두 shader를 구분하기 위해 일반적으로 사용됩니다.

**`shader.vert`**의 내용은 아래와 같아야 합니다.

```glsl
#version 450

layout(location = 0) out vec3 fragColor;

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}
```

그리고 **`shader.frag`**의 내용은 아래와 같아야 합니다.

```glsl
#version 450

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
```

이제 우리는 **`glslc`**프로그램을 사용하여 이것들을 SPIR-V bytecode로 컴파일 할 겁니다.



## Windows

아래의 내용으로 **`compile.bat`** 파일을 생성합니다.

```basic
C:/VulkanSDK/x.x.x.x/Bin32/glslc.exe shader.vert -o vert.spv
C:/VulkanSDK/x.x.x.x/Bin32/glslc.exe shader.frag -o frag.spv
pause
```

`glslc.exe`의 경로를 Vulkan SDK를 설치한 경로로 바꾸십시오. 이 파일을 더블클릭해서 실행하십시오



## Linux

아래의 내용으로 **`compile.sh`** 파일을 생성합니다.

```basic
/home/user/VulkanSDK/x.x.x.x/x86_64/bin/glslc shader.vert -o vert.spv
/home/user/VulkanSDK/x.x.x.x/x86_64/bin/glslc shader.frag -o frag.spv
```

**`glslc`**의 경로를 Vulkan SDK를 설치한 경로로 바꾸십시오. 스크립트를 **`chmod + x comiple.sh`**로 실행가능하도록 만든 뒤 실행하십시오.



### 플랫폼 별 지침은 여기서 끝

두 명령은 컴파일러에게 GLSL source file를 읽고 -o (output) flag를 사용하여 SPIR-V bytecode로 출력해 달라고 요청합니다.

shader에 문법 오류가 있으면 컴파일러에서 줄 번호와 문제를 알려줍니다. 세미콜론을 빼먹을 채로 컴파일 스크립트를 다시 실행해 보십시오. 어떤 인자도 사용하지 않고 컴파일러를 실행해 보면 지원하는 플래그의 종류를 확인 할 수 있습니다. 예를 들어 bytecode를 사람이 읽을 수 있는 포맷으로 출력하여 shader가 무엇을 하고 있는지, 어떤 최적화가 이 단계에서 적용됬는지를 정확하게 확인할 수 있습니다.

커맨드라인에서 shader를 컴파일하는 것은 가장 간단한 옵션 중 하나이며 이 튜토리얼에서 사용할 옵션이지만, 사용자 코드에서 직접 shader를 컴파일 할 수도 있습니다. Vulkan SDK에는 프로그램 내에서 GLSL을 SPIR-V로 컴파일 하는 **[libshaderc](https://github.com/google/shaderc)**라는 라이브러리가 포함되어 있습니다.



# Loading a shader

이제 우리는 SPIR-V shader를 생성하는 방법을 알았습니다. 이번 시간에는 특정 시점에 그들을 graphics pipeline에 연결시키기 위해 shader를 우리 프로그램에 로드합니다. 먼저 파일에서 바이너리 데이타를 로드하는 헬퍼 함수를 작성합니다.

```cpp
#include <fstream>
...
static std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}
}
```

**`readfile`**함수는 지정된 파일에서 모든 byte를 읽고 **`std::vector`**로 관리되는 byte 배열에 넣어서 반환할 것입니다. 파일을 두개의 플래그로 오픈하는것 부터 시작합니다.

- **`ate`** : 파일에 끝에서 읽음
- **`binary`** : 바이너리로 파일을 읽는다.(텍스트 변환을 방지)

파일의 끝에서 읽기를 시작하는 것의 장점은 읽기 위치로 파일의 사이즈를 결정하고 버퍼를 할당할 수 있다는 점입니다.

```cpp
size_t fileSize = (size_t)file.tellg();
std::vector<char> buffer(fileSize);
```

이후 파일의 시작을 찾아 뒤로 가서 한번에 모든 바이트를 읽을 수 있습니다.

```cpp
file.seekg(0);
file.read(buffer.data(), fileSize);
```

마지막으로 파일을 닫고 바이트 데이터를 리턴합니다.

```cpp
file.close();
return buffer;
```

이제 이 함수를 **`createGraphicsPipeline`**에서 호출하여 bytecode로 된 두 shader를 로드합니다

```cpp
void createGraphicsPipeline() 
{
	auto vertShaderCode = readFile("shaders/vert.spv");
	auto fragShaderCode = readFile("shaders/frag.spv");
}
```

shader가 정확하게 로드되었는지 확인하기 위해서 buffer의 사이즈를 출력하고 실제 파일 사이즈와 같은지 체크합니다. binary code이므로 code에서 null 로 종료할 필요는 없으며 나중에 크기에 대해 명시적으로 설명하도록 하겠습니다.



# Creating shader modules

pipeline에 코드를 전달하기 전에 이 코드를 **[VkShaderModule](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkShaderModule.html)** 오브젝트로 래핑해야 합니다. 이를 수행하는 헬퍼 함수인 **`createShaderModule`**을 생성합니다.

```cpp
VkShaderModule createShaderModule(const std::vector<char>& code) {
}
```

이 함수는 bytecode로 된 buffer를 파라미터로 받아들이고 이를 가지고 **[VkShaderModule](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkShaderModule.html)**를 생성합니다.

Shader module를 만드는 것은 간단합니다. bytecode로 된 buffer의 포인터와 buffer의 길이를 지정하기만 하면 됩니다. 이 정보는 **[VkShaderMoudleCreateInfo](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkShaderModuleCreateInfo.html)** 구조체에 지정됩니다. 하나 봐야 할 부분은 bytecode의 크기는 byte들로 지정되지만, bytecode의 포인터 타입이 **`char`** 형이 아니라 **`uint32_t`**형 이라는 것입니다. 그래서 아래에서 처럼 **`reinterpret_cast`**를 통해 포인터를 형변환해야 합니다. 이렇게 형변환을 할때 데이터가 **`uint32_t`** 타입의 정렬 요구사항을 만족하는지 확인해야 합니다. 운 좋게도 데이터가 저장되어 있는 **`std::vector`** 컨테이너의 기본 할당자가 이미 최악의 정렬 요구사항에서도 데이터가 적합하도록 보장합니다.

```cpp
VkShaderModuleCreateInfo createInfo = {};
createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
createInfo.codeSize = code.size();
createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
```

그런 다음, **[VkshaderMoulde](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkShaderModule.html)**은 **[vkCreateShaderMoulde](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCreateShaderModule.html)** 호출로 생성될 수 있습니다.

```cpp
VkShaderModule shaderModule;
if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
	throw std::runtime_error("failed to create shader module!");
}
```

파라미터는 이전에 오브젝트를 생성하는 함수와 같습니다. 논리 디바이스, 생성 정보 구조체의 포인터, 선택적 사용자 정의 할당자 포인터, 출력 변수의 핸들. 코드에서 사용된 buffer는 shader mudule를 생성한 후에 즉시 해제할 수 있습니다. 생성된 shader module를 반환하는 걸 잊지 마십시오.

```cpp
return shaderModule;
```

shader module은 우리가 이전에 파일에서 로드하고 함수가 정의된 shader bytecode를 얇은 래퍼일 뿐입니다. graphics pipeline이 생성되기 전까지 GPU는 SPIR-V bytecode를 machine code로 컴파일하고 링킹하는 작업을 수행하지 않습니다. 이 말은 graphics pipeline 생성이 완료되는 대로 즉시 shader moudle를 폐기할 수 있다는 의미입니다. 이것이 이들을 클래스 멤버가 아닌 `createGraphicsPipeline`의 local 변수로 만든 이유입니다.

```cpp
void createGraphicsPipeline() {
	auto vertShaderCode = readFile("shaders/vert.spv");
	auto fragShaderCode = readFile("shaders/frag.spv");

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
```

그 후 함수의 마지막에서 두번의 [vkDestroyShaderModule](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkDestroyShaderModule.html) 호출을 통해 정리되어야 합니다. 이 라인 앞에 이 장의 나머지 모든 코드가 삽입됩니다.

```cpp
...
vkDestroyShaderModule(device, fragShaderModule, nullptr);
vkDestroyShaderModule(device, vertShaderModule, nullptr);
```



# Shader stage creation

shader를 실제 사용하기 위해서 우리는 실제 pipeline 생성 프로세스의 일부분으로 **[VkPipelineShaderStageCreateInfo](https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkPipelineShaderStageCreateInfo.html)** 구조체를 통해 shader를 pipeline state에 연결해야 합니다.

우리는 다시 **`createGraphicspipeline`** 함수로 가서 vertex shader를 위해 이 구조체를 채우기 시작할 겁니다.

```cpp
VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
```

의무적인 **`sType`** 멤버를 제외한 첫번째 단계(stage)는 Vulkan에게 shader가 사용될 pipeline 단계를 알려줍니다. 여기에는 이전 챕터에서 설명한 각각의 프로그래머블 단계에 대한 enum 값이 있습니다.

```cpp
vertShaderStageInfo.module = vertShaderModule;
vertShaderStageInfo.pName = "main";
```

다음 두 멤버는 코드를 가진 shader moudle과 호출해야 할 함수(***entrypoint***로 알려져 있습니다.)를 지정합니다. 

이건 단일 shader module안에 여러개의 fragment shader가 결합되고 서로 다른 진입점(entry point)을 이용하여 그들의 동작을 구별할 수 있다는 의미입니다. 하지만 여기서 우린 표준인 **`main`**을 유지할 것입니다.

추가로 선택적 멤버인 **`pSpecializationInfo`**가 하나 있습니다. 여기선 이걸 사용하지 않을거지만, 논의 할 가치는 있습니다. 

이 멤버는 shader를 위한 상수 값을 지정할 수 있게 해줍니다. 이를 이용하면 단일 shader moudle을 사용하여 shader에서 사용되는 상수에 서로 다른 값을 지정하여 pipeline 생성시점에 동작을 구성할 수 있습니다. 이는 render time에 변수를 사용하여 shader를 구성하는 것 보다 효율적입니다. 이는 컴파일러가 이런 변수에 의존하는 **`if`** 구문을 삭제하는 것과 같은 최적화 작업을 할 수 있기 때문입니다. 
아래처럼 아무런 상수가 없다면 이 멤버를 **`nullptr`**로 설정하면 되는데 이런 구조체 초기화는 자동적으로 수행됩니다.

구조체를 fragment shader에 알맞게 수정하는 건 쉽습니다.

```cpp
VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
fragShaderStageInfo.module = fragShaderModule;
fragShaderStageInfo.pName = "main";
```

이 두 구조체를 가진 배열의 정의하며 마무리 합니다. 

이 두 구조체는 나중에 실제 pipeline 생성 단계에서 참조하기 위해 사용할 것입니다.

```cpp
VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
```

이것이 pipeline의 프로그래머블 가능 단계에 대한 설명 전부입니다. 다음 챕터에서는 fixed-function 단계에 대해 알아볼 것입니다.

**[C++ code](https://vulkan-tutorial.com/code/09_shader_modules.cpp)** / **[Vertex shader](https://vulkan-tutorial.com/code/09_shader_base.vert)** / **[Fragment shader](https://vulkan-tutorial.com/code/09_shader_base.frag)**



> 세이더 동작과 관련된 질문이 있어요. 왜 color가 자동으로 보간되죠? 예를 들어 그라디언트를 더 빨간색으로 만드려면 어떻게 해야 하나요? 
>
> > 이것(보간)은 세이더가 기본적으로 동작하는 방식이며, 텍스쳐 좌표등에 유용합니다. [type qualifier](https://www.khronos.org/opengl/wiki/Type_Qualifier_(GLSL))를 사용하여 조금인지만 보간을 제어할 수 있습니다.물론 그라디언트의 특성을 변경하는 지수 함수와 같이 보간된 값에 고유한 변환을 적용할 수도 있습니다.



- https://github.com/SaschaWillems/Vulkan 에서 vulkan geometry shader example를 찾을 수 있습니다. 그 외에도 여러가지 예제들이 있어요.
