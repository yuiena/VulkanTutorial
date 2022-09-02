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
vertex shader를 다시 컴파일 하십시오

