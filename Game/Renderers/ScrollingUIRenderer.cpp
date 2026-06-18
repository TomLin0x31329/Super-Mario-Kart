#include "ScrollingUIRenderer.h"

#include "Core/TextureUtils.hpp"
#include "Util/Logger.hpp"

#include <glm/gtc/type_ptr.hpp>


namespace Game::Renderer
{
    void ScrollingUIRenderer::Init()
    {
        program_ = std::make_unique<Core::Program>(
            RESOURCE_DIR "/shaders/ScrollingUI.vert",
            RESOURCE_DIR "/shaders/ScrollingUI.frag");
        program_->Bind();

        GLint texLoc = glGetUniformLocation(program_->GetId(), "u_Surface");
        glUniform1i(texLoc, 0);


        vertexArray_ = std::make_unique<Core::VertexArray>();
        vertexArray_->AddVertexBuffer(std::make_unique<Core::VertexBuffer>(
            std::vector<float>{ 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f }, 2));
        vertexArray_->AddVertexBuffer(std::make_unique<Core::VertexBuffer>(
            std::vector<float>{ 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f }, 2));
        vertexArray_->SetIndexBuffer(std::make_unique<Core::IndexBuffer>(
            std::vector<unsigned int>{ 0, 1, 2, 0, 2, 3 }));
    }
    void ScrollingUIRenderer::Draw(std::shared_ptr<Util::Image> image,
        float x, float y, float width, float height,
        float uvOffsetX, float uvOffsetY,
        bool flipX, bool flipY, float alpha)
    {
        program_->Bind();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, image->GetTextureId());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glm::vec2 imgSize = image->GetSize();
        float uvScaleY = 1.0f;
        float uvScaleX = (width / height) / (imgSize.x / imgSize.y);

        // 傳遞 UV Scale 與 Offset 與 Alpha
        glUniform2f(glGetUniformLocation(program_->GetId(), "u_UVScale"), uvScaleX, uvScaleY);
        glUniform2f(glGetUniformLocation(program_->GetId(), "u_UVOffset"), uvOffsetX, uvOffsetY);
        glUniform1f(glGetUniformLocation(program_->GetId(), "u_Alpha"), alpha);

        // 正交投影矩陣
        glm::mat4 projection = glm::ortho(0.0f, 256.0f, 224.0f, 0.0f, -1.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(
            program_->GetId(), "u_Projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // 傳遞 Model 矩陣
        glm::mat4 model = glm::mat4(1.0f);
        float translateX = flipX ? (x + width) : x;
        float translateY = flipY ? (y + height) : y;
        model = glm::translate(model, glm::vec3(translateX, translateY, 0.0f));
        float scaleX = flipX ? -width : width;
        float scaleY = flipY ? -height : height;
        model = glm::scale(model, glm::vec3(scaleX, scaleY, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(
            program_->GetId(), "u_Model"), 1, GL_FALSE, glm::value_ptr(model));

        // 畫出來
        vertexArray_->Bind();
        vertexArray_->DrawTriangles();
    }
}
