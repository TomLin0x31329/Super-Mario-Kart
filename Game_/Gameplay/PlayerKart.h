#pragma once
#include "Util/Logger.hpp"

#include "pch.hpp"
#include <glm/glm.hpp>
#include <string>
#include <cmath>
#include <algorithm>
#include <vector>
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"

#include <iostream>

#include "GameCore/LevelData.h"

namespace Game::Gameplay
{
    struct AccelTier
    {
        float speedLimit;
        float accelRate;
    };

    class PlayerKart
    {
    public:
        glm::vec2 Position = { 920.0f * 1.6f, 589.0f * 1.6f };
        glm::vec2 ExpectedDir = { 0.0f, -1.0f }; // 車頭朝向向量
        glm::vec2 Velocity = { 0.0f, -0.01f };   // 真實移動向量 (包含速度大小)

        // --- LOD 區間加速度 ---
        std::vector<AccelTier> AccelerationCurve = {
            { 150.0f, 240.0f },  // 起步：加速度適中，好控車
            { 450.0f, 420.0f },  // 中段：爆發力強，推背感
            { 9999.f, 150.0f }   // 尾速：加速度銳減，很難到達極速
        };

        float MaxSpeed = 400.0f;
        float Deceleration = 200.0f; // 自然滑行減阻
        float BrakePower = 600.0f;   // 煞車力度

        // --- 轉向與側滑參數 ---
        float TurnRate = 2.0f; // 方向盤轉速 (弧度/秒)

        // 🌟 移動向量跟隨轉向的比例 (核心魔法)
        // 1.0 = 不打滑的軌道車，0.5 = 移動方向只跟著轉一半
        float NormalTurnFactor = 0.4f;
        float DriftTurnFactor = 1.8f;  // 甩尾時，車身8往外拋的感覺極大！

        float Grip = 12.0f; // 抓地力：放開方向鍵後，真實移動方向拉回車頭方向的速度
        float BounceFactor = 0.6f; // 🌟 新增：反彈係數 (0.0 = 不反彈，1.0 = 完美彈性碰撞)

        std::vector<std::shared_ptr<Util::Image>> Sprites;
        std::shared_ptr<Util::Image> CurrentSprite = nullptr;
        bool ShouldFlipSprite = false;

    private:
        const std::vector<std::pair<float, int>> marioKartSpritesLUT = {
            {   0.0f,  1 }, // 1.PNG: 正前方 (0°)
            {  15.0f,  2 }, // 2.PNG
            {  30.0f,  3 }, // 3.PNG
            {  45.0f,  4 }, // 4.PNG
            {  60.0f,  5 }, // 5.PNG
            {  75.0f,  6 }, // 6.PNG
            {  90.0f,  7 }, // 7.PNG: 正右側 (90°)
            { 105.0f,  8 }, // 8.PNG
            { 120.0f,  9 }, // 9.PNG
            { 135.0f, 10 }, // 10.PNG
            { 150.0f, 11 }, // 11.PNG
            { 180.0f, 12 }  // 12.PNG: 正後方 (180°)
        };

    private:
        glm::vec2 RotateVector(glm::vec2 v, float angleRad)
        {
            float c = std::cos(angleRad);
            float s = std::sin(angleRad);
            return { v.x * c - v.y * s, v.x * s + v.y * c };
        }
        float GetAngleDifference(float target, float current)
        {
            float diff = std::fmod(target - current, 2.0f * M_PI);
            if (diff > M_PI) diff -= 2.0f * M_PI;
            if (diff < -M_PI) diff += 2.0f * M_PI;
            return diff;
        }

    public:
        void Init()
        {
            for (int i = 1; i <= 12; ++i)
            {
                Sprites.push_back(std::make_shared<Util::Image>(
                    RESOURCE_DIR "/sprites/karts/mario_kart/lod_0/" + std::to_string(i) + ".PNG"));
            }
            CurrentSprite = Sprites[0]; // 預設顯示第一張
        }
        void Update(float dt, const Game::GameCore::CollisionMap& colMap)
        {
            float currentSpeed = glm::length(Velocity);
            float currentMaxSpeed = MaxSpeed;
            float currentDecel = Deceleration;

            // ==========================================
            // 🌟 輔助函式：判斷座標是否為牆壁或草地
            // ==========================================
            auto CheckTerrain = [&](float testX, float testY, bool& outIsWall, bool& outIsGrass) {
                outIsWall = false;
                outIsGrass = false;
                int mapX = static_cast<int>(testX / 1.6f);
                int mapY = static_cast<int>(testY / 1.6f);
                uint8_t r = 0, g = 0, b = 0;

                if (colMap.GetPixelColor(mapX, mapY, r, g, b)) {
                    if (r == 0 && g == 0 && b == 0) outIsGrass = true;
                    else if (r == 104 && g == 104 && b == 248) outIsWall = true;
                }
                else {
                    outIsWall = true; // 掉出地圖外視同撞牆
                }
                };

            // 檢查玩家「當前腳下」是否在草地
            bool isWall = false, isGrass = false;
            CheckTerrain(Position.x, Position.y, isWall, isGrass);
            if (isGrass) {
                currentMaxSpeed = MaxSpeed * 0.4f;
                currentDecel = Deceleration * 4.0f;
            }

            // ... (保留你原本處理轉向、油門、抓地力 Velocity 計算的程式碼) ...
            // [從 bool isDrifting = ... 一直到 Velocity = glm::mix(...);]

            bool isDrifting = Util::Input::IsKeyPressed(Util::Keycode::SPACE);
            float turnFactor = isDrifting ? DriftTurnFactor : NormalTurnFactor;

            if (currentSpeed > 10.0f)
            {
                float turnAngle = 0.0f;
                if (Util::Input::IsKeyPressed(Util::Keycode::A) || Util::Input::IsKeyPressed(Util::Keycode::LEFT)) turnAngle -= TurnRate * dt;
                if (Util::Input::IsKeyPressed(Util::Keycode::D) || Util::Input::IsKeyPressed(Util::Keycode::RIGHT)) turnAngle += TurnRate * dt;

                if (turnAngle != 0.0f) {
                    ExpectedDir = RotateVector(ExpectedDir, turnAngle);
                    ExpectedDir = glm::normalize(ExpectedDir);
                    Velocity = RotateVector(Velocity, turnAngle * turnFactor);
                }
            }

            if (Util::Input::IsKeyPressed(Util::Keycode::W) || Util::Input::IsKeyPressed(Util::Keycode::UP))
            {
                float currentAccel = 0.0f;
                for (const auto& tier : AccelerationCurve) {
                    if (currentSpeed <= tier.speedLimit) { currentAccel = tier.accelRate; break; }
                }
                Velocity += ExpectedDir * currentAccel * dt;
            }
            else if (Util::Input::IsKeyPressed(Util::Keycode::S) || Util::Input::IsKeyPressed(Util::Keycode::DOWN))
            {
                if (currentSpeed > 0.1f) Velocity -= glm::normalize(Velocity) * BrakePower * dt;
            }

            if (currentSpeed > 0.1f && !(Util::Input::IsKeyPressed(Util::Keycode::W) || Util::Input::IsKeyPressed(Util::Keycode::UP)))
            {
                float dragDrop = Deceleration * dt;
                if (dragDrop > currentSpeed) dragDrop = currentSpeed;
                Velocity -= glm::normalize(Velocity) * dragDrop;
            }

            if (glm::length(Velocity) > currentMaxSpeed)
            {
                Velocity = glm::mix(Velocity, glm::normalize(Velocity) * currentMaxSpeed, 5.0f * dt);
            }

            // 循跡抓地力
            currentSpeed = glm::length(Velocity);
            if (currentSpeed > 10.0f)
            {
                float currentGrip = isDrifting ? Grip * 0.1f : Grip;
                glm::vec2 idealVelocity = ExpectedDir * currentSpeed;
                Velocity = glm::mix(Velocity, idealVelocity, currentGrip * dt);
            }

            // ==========================================
            // 🌟 核心魔法：分離軸移動與反彈 (Separating Axis)
            // ==========================================

            // 1. 先嘗試在 X 軸上移動
            float nextX = Position.x + Velocity.x * dt;
            CheckTerrain(nextX, Position.y, isWall, isGrass);
            if (isWall)
            {
                std::cout << "Hit wall!" << std::endl;
                // 撞牆了！X 軸不准前進，且 X 軸速度反轉並衰減
                Velocity.x = -Velocity.x * BounceFactor;

                // 可選：撞牆時讓車頭也稍微偏轉，產生失控感
                ExpectedDir = RotateVector(ExpectedDir, (Velocity.x > 0 ? 0.2f : -0.2f));
            }
            else
            {
                // 沒撞牆，正式更新 X 座標
                Position.x = nextX;
            }

            // 2. 再嘗試在 Y 軸上移動
            float nextY = Position.y + Velocity.y * dt;
            CheckTerrain(Position.x, nextY, isWall, isGrass);
            if (isWall)
            {
                std::cout << "Hit wall!" << std::endl;

                // 撞牆了！Y 軸不准前進，且 Y 軸速度反轉並衰減
                Velocity.y = -Velocity.y * BounceFactor;

                ExpectedDir = RotateVector(ExpectedDir, (Velocity.y > 0 ? 0.2f : -0.2f));
            }
            else
            {
                // 沒撞牆，正式更新 Y 座標
                Position.y = nextY;
            }

            // ==========================================
            // 更新動畫
            // ==========================================
            UpdateSprite();
        }

    private:
        void UpdateSprite()
        {
            float currentSpeed = glm::length(Velocity);
            if (currentSpeed < 5.0f)
            {
                CurrentSprite = Sprites[0];
                ShouldFlipSprite = false;
                return;
            }

            // 1. 計算「車頭向量 (ExpectedDir)」和「攝影機前進向量 (Velocity)」之間的夾角
            // 因為鏡頭是跟隨 Velocity 的。
            float expectedAngle = std::atan2(ExpectedDir.x, -ExpectedDir.y);
            float actualAngle = std::atan2(Velocity.x, -Velocity.y);

            // 2. 計算兩者夾角 (Delta Angle)
            float deltaAngle = GetAngleDifference(expectedAngle, actualAngle);

            // 3. 轉換為角度 (Degree) 
            float deltaDeg = deltaAngle * 180.0f / M_PI;

            // 4. 🌟 核心： Mario Kart 圖形翻轉邏輯 🌟
            // 我們假設 1-12.PNG 提供了從 0° (正面) 到 180° (背面) 的順時鐘視角。

            // 將所有角度（正和負）對映到 0 到 180 度的範圍內，並決定是否需要翻轉。
            float lookupAngle = 0.0f;
            if (deltaDeg < 0)
            {
                // 負角度（例如 -45°，車頭偏向左側視角）：
                // 對映到對應的正角度進行查找（例如 45°），並標記需要水平翻轉。
                lookupAngle = std::abs(deltaDeg);
                ShouldFlipSprite = true;
            }
            else
            {
                // 正角度（例如 45°，車頭偏向右側視角）：
                // 直接使用正角度查找，不需要翻轉。
                lookupAngle = deltaDeg;
                ShouldFlipSprite = false;
            }

            // 防呆：確保 lookupAngle 不會超過查找表的範圍 (180°)
            lookupAngle = std::clamp(lookupAngle, 0.0f, 180.0f);

            // 5. 🌟 核心：二分查找不均勻角度 LUT (std::lower_bound) 🌟
            // std::lower_bound 會尋找「不小於」 lookupAngle 的第一個元素。
            // 意味著它會自動找到最接近的「向上」門檻。
            auto it = std::lower_bound(marioKartSpritesLUT.begin(), marioKartSpritesLUT.end(), lookupAngle,
                [](const std::pair<float, int>& a, float b) {
                    return a.first < b; // 比較 pair 的角度和目標角度
                });

            int spriteIndex = 1;
            if (it == marioKartSpritesLUT.end()) {
                // 如果 lookupAngle 比表中所有角度都大 (雖然有防呆，還是加上)，選擇最後一張圖。
                spriteIndex = marioKartSpritesLUT.back().second;
            }
            else {
                // 否則，使用找到的圖片索引。
                spriteIndex = it->second;
            }

            // 6. 組合最終的圖片路徑
            CurrentSprite = Sprites[spriteIndex - 1];
        }

    };
}
