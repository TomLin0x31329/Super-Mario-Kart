#include "Renderers/UIRenderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


namespace Game::Renderer
{
    UIRenderer::~UIRenderer()
    {
        if (quadVAO_ != 0) {
            glDeleteVertexArrays(1, &quadVAO_);
            glDeleteBuffers(1, &quadVBO_);
        }
    }

    void UIRenderer::Init()
    {
        // 1. 建立 1x1 單位方形 (原點在左上角)
        float vertices[] = {
            0.0f, 1.0f,   0.0f, 1.0f, // 左下
            1.0f, 0.0f,   1.0f, 0.0f, // 右上
            0.0f, 0.0f,   0.0f, 0.0f, // 左上

            0.0f, 1.0f,   0.0f, 1.0f, // 左下
            1.0f, 1.0f,   1.0f, 1.0f, // 右下
            1.0f, 0.0f,   1.0f, 0.0f  // 右上
        };

        glGenVertexArrays(1, &quadVAO_);
        glGenBuffers(1, &quadVBO_);
        glBindVertexArray(quadVAO_);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // 2. 共用與 Billboard 相同的 UI Shader
        program_ = std::make_unique<Core::Program>(
            RESOURCE_DIR "/shaders/ui.vert",
            RESOURCE_DIR "/shaders/ui.frag"
        );
    }
    void UIRenderer::DrawImage(std::shared_ptr<Util::Image> image, float x, float y, bool flipH)
    {
        if (!image) return;

        // 將繪製指令塞入本幀的佇列中
        drawQueue_.push_back({
            x, y,
            image->GetSize().x,
            image->GetSize().y,
            image->GetTextureId(),
            flipH
            });
    }
    void UIRenderer::Render()
    {
        if (drawQueue_.empty()) return;

        // 開啟混和與關閉深度測試 (UI 永遠在最上層)
        program_->Bind();
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        // 設定正交投影 (左上角 0,0 -> 右下角 256,224)
        glm::mat4 projection = glm::ortho(0.0f, 256.0f, 224.0f, 0.0f, -1.0f, 1.0f);
        GLint projLoc = glGetUniformLocation(program_->GetId(), "u_Projection");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(quadVAO_);
        GLint modelLoc = glGetUniformLocation(program_->GetId(), "u_Model");
        GLint texLoc = glGetUniformLocation(program_->GetId(), "u_Texture");
        glUniform1i(texLoc, 0);

        // 依序執行佇列中的所有繪製指令
        for (const auto& cmd : drawQueue_)
        {
            glm::mat4 model = glm::mat4(1.0f);
            if (cmd.flipH)
            {
                model = glm::translate(model, glm::vec3(cmd.x + cmd.width, cmd.y, 0.0f));
                model = glm::scale(model, glm::vec3(-cmd.width, cmd.height, 1.0f));
            }
            else
            {
                model = glm::translate(model, glm::vec3(cmd.x, cmd.y, 0.0f));
                model = glm::scale(model, glm::vec3(cmd.width, cmd.height, 1.0f));
            }
            
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cmd.textureId);

            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // 🌟 立即模式的核心：畫完就清空！
        drawQueue_.clear();
    }
}
