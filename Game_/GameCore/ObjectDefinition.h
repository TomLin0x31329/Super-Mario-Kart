#pragma once

#include "pch.hpp"

#include "Util/Image.hpp"

namespace Game::GameCore
{
    struct DirectionalSprite
    {
        float angle;
        std::shared_ptr<Util::Image> image;
    };
    struct SpriteLOD
    {
        float maxDistance;
        std::vector<DirectionalSprite> sprites;
    };
    struct ObjectDefinition
    {
        float collisionRadius = 16.0f;
        std::vector<SpriteLOD> lods;
    };
}
