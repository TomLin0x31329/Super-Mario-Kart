#version 410 core

layout(location = 0) in vec2 screenUV; 
layout(location = 0) out vec4 fragColor;

uniform sampler2D u_Surface;
uniform sampler2D u_OutOfBoundsTile;

// 🌟 變數統一改成純像素與直覺命名
uniform vec2 u_CameraPos;    // 攝影機 XZ 座標 (純像素)
uniform float u_CameraHeight;// 攝影機高度 (純像素，影響看多遠)
uniform float u_CameraYaw;   // 攝影機朝向 (弧度)
uniform float u_CameraPitch;
uniform vec2 u_TrackSize;    // 地圖真實尺寸 (例如 1024x1024)

void main() 
{
    float horizon = 1.0 + u_CameraPitch;
    float y = horizon - screenUV.y;
    
    if (y <= 0.001) {
        fragColor = texture(u_OutOfBoundsTile, fract(screenUV * 16.0));
        return;
    }

    // 1. 計算深度 (因為 u_CameraHeight 是像素，算出來的 z 就是真實的像素距離)
    float z = u_CameraHeight / y;

    // 2. 螢幕水平映射 (fov 控制視野，1.0 剛好，越小畫面越放大)
    float fov = 0.42; 
    float x = (screenUV.x - 0.5) * 2.0 * fov; 

    // ====================================================
    // 3. 🌟 確保 Shader 的前後左右與 C++ 完全一致 🌟
    // ====================================================
    // C++ 的 forwardDir 是 { sin(yaw), 0, -cos(yaw) }
    vec2 forward = vec2(sin(u_CameraYaw), -cos(u_CameraYaw));
    
    // C++ 的 rightDir 是 { cos(yaw), 0, sin(yaw) }
    vec2 right = vec2(cos(u_CameraYaw), sin(u_CameraYaw));
    
    // 構造這條射線的真正方向 (往前 1.0，往右 x)
    vec2 rayDir = forward * 1.0 + right * x;

    // 4. 算出打在地圖上的真實像素座標 (攝影機位置 + 射線方向 * 距離)
    vec2 worldPos = u_CameraPos + rayDir * z;

    // ====================================================
    // 5. 像素轉 UV 與繪製
    // ====================================================
    float mapScale = 1.6; // 如果你想讓整個賽道面積變大 2 倍，就調大這個數字
    vec2 worldUV = (worldPos / u_TrackSize) / mapScale;

    float grassTiling = 128.0;
    if (worldUV.x < 0.0 || worldUV.x > 1.0 || worldUV.y < 0.0 || worldUV.y > 1.0) {
        fragColor = texture(u_OutOfBoundsTile, fract(worldUV * grassTiling));
    } 
    else {
        vec4 texColor = texture(u_Surface, worldUV);
        if (texColor.a < 0.01) {
            fragColor = texture(u_OutOfBoundsTile, fract(worldUV * grassTiling));
        } else {
            fragColor = texColor;
        }
    }
}
