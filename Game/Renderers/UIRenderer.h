#pragma once

#include "pch.hpp"
#include <vector>
#include <memory>
#include <GL/glew.h>
#include <glm/glm.hpp>

#include "Core/Program.hpp"
#include "Util/Image.hpp"

namespace Game::Renderer
{
    class UIRenderer
    {
    private:
        GLuint quadVAO_ = 0;
        GLuint quadVBO_ = 0;
        std::unique_ptr<Core::Program> program_;

        struct UIDrawCommand
        {
            float x, y;
            float width, height;
            GLuint textureId;
            bool flipH;
        };
        std::vector<UIDrawCommand> drawQueue_;

    public:
        UIRenderer() = default;
        UIRenderer(const UIRenderer&) = delete;
        UIRenderer& operator=(const UIRenderer&) = delete;
        ~UIRenderer();

    public:
        void Init();
        void DrawImage(std::shared_ptr<Util::Image> image, float x, float y, bool flipH = false);
        void Render();

    };
}
