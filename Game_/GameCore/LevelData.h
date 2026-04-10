#pragma once

#include "pch.hpp"
#include <glm/glm.hpp>

namespace Game::GameCore
{
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
    };
}
