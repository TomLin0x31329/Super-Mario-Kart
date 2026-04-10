#version 410 core

layout(location = 0) in vec2 vertPosition;
layout(location = 1) in vec2 vertUv;

layout(location = 0) out vec2 screenUV;

void main() 
{
    // 直接將頂點貼死在畫面上 (Clip Space: -1.0 ~ 1.0)
    gl_Position = vec4(vertPosition, 0.0, 1.0);
    screenUV = vertUv;
}
