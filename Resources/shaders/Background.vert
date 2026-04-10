#version 410 core

layout(location = 0) in vec2 vertPosition;
layout(location = 1) in vec2 vertUv;

layout(location = 0) out vec2 uv;

uniform float u_YawOffset;   
uniform vec2 u_Resolution; // Viewport 尺寸 (例如 256x21)
uniform vec2 u_TexSize;    // 圖片真實尺寸 (例如 256x256)

void main() {
    gl_Position = vec4(vertPosition, 0.0, 1.0); 

    // 🌟 1:1 像素縮放比例
    // 例如 height: 21 / 256 = 0.082
    vec2 scale = u_Resolution / u_TexSize;

    // X 軸：加上旋轉偏移，然後乘上比例 (維持水平不變形)
    float finalUvX = (vertUv.x + u_YawOffset) * scale.x;

    // Y 軸：核心魔法「對齊底部 (地平線)」
    // 讓 Viewport 的底部 (vertUv.y = 1.0) 剛好對應到圖片的最底部 (1.0)
    // Viewport 的頂部則對應到 1.0 - scale.y (例如 1.0 - 0.082 = 0.918)
    float finalUvY = 1.0 - scale.y + (vertUv.y * scale.y);

    uv = vec2(finalUvX, finalUvY);
}