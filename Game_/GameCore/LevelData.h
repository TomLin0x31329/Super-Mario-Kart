#pragma once

#include "pch.hpp"
#include <glm/glm.hpp>

namespace Game::GameCore
{
    struct CollisionMap
    {
        int width = 0;
        int height = 0;
        std::vector<uint8_t> pixels;

        bool GetPixelColor(int x, int y, uint8_t& r, uint8_t& g, uint8_t& b) const
        {
            if (x < 0 || x >= width || y < 0 || y >= height) return false;

            int index = (y * width + x) * 4;
            r = pixels[index];
            g = pixels[index + 1];
            b = pixels[index + 2];
            return true;
        }
    };
    struct StartingPosition
    {
        int id;
        glm::vec2 pos;
    };
    struct ItemBoxData
    {
        glm::vec2 pos;
        int direction;
    };

    struct LevelData
    {
        std::string name;
        int totalLaps = 3;
        float initialYaw = 0.0f;

        std::vector<StartingPosition> startingGrid;

        std::vector<glm::vec2> coins;
        std::vector<ItemBoxData> itemBoxes;

        std::vector<glm::vec2> pipes;

        CollisionMap collisionMap;
    };
}
