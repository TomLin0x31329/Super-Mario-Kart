#pragma once

#include "GameCore/LevelData.h"
#include <string>

namespace Game::Systems
{
    class LevelLoader
    {
    public:
        static Game::GameCore::LevelData LoadLevel(const std::string& levelPath);

    };
}
