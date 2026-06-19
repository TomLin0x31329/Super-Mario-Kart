#include "BackgroundRenderer.h"

#include "Core/TextureUtils.hpp"
#include "Util/Logger.hpp"


namespace Game::Renderer
{
	BackgroundRenderer::BackgroundRenderer(const std::string& imagePath, float scrollSpeed, glm::vec2 viewport)
		: imagePath_(imagePath), scrollSpeedMultiplier_(scrollSpeed), viewport_(viewport)
	{

	}

	bool BackgroundRenderer::Init()
	{
        // 加載 Shader
        program_ = std::make_unique<Core::Program>(
            RESOURCE_DIR "/shaders/Background.vert",
            RESOURCE_DIR "/shaders/Background.frag");
        program_->Bind();

        GLint texLoc = glGetUniformLocation(program_->GetId(), "u_Surface");
        glUniform1i(texLoc, 0);

        // 建立全螢幕 2D 矩形
        vertexArray_ = std::make_unique<Core::VertexArray>();
        vertexArray_->AddVertexBuffer(std::make_unique<Core::VertexBuffer>(
            std::vector<float>{ -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f }, 2));
        vertexArray_->AddVertexBuffer(std::make_unique<Core::VertexBuffer>(
            std::vector<float>{ 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f }, 2));
        vertexArray_->SetIndexBuffer(std::make_unique<Core::IndexBuffer>(
            std::vector<unsigned int>{ 0, 1, 2, 0, 2, 3 }));

        // 加載 Texture
        SDL_Surface* surface = IMG_Load(imagePath_.c_str());
        if (!surface)
        {
            LOG_ERROR("Failed to load Prop texture: {}", IMG_GetError());
            return false;
        }
        textureSize_ = glm::vec2((float)surface->w, (float)surface->h);
        texture_ = std::make_unique<Core::Texture>(
            Core::SdlFormatToGlFormat(surface->format->format),
            surface->w, surface->h, surface->pixels);
        SDL_FreeSurface(surface);

        return true;
	}
    void BackgroundRenderer::Draw(float offset)
    {
        // 設定使用的 Shader 和 Texture
        program_->Bind();
        glActiveTexture(GL_TEXTURE0);
        texture_->Bind(0);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // 計算參數
        float parallaxYaw = offset * scrollSpeedMultiplier_;
        float uvOffset = parallaxYaw / (2.0f * (float)M_PI);

        // 傳遞參數給 Shader
        GLint offsetLoc = glGetUniformLocation(program_->GetId(), "u_YawOffset");
        glUniform1f(offsetLoc, uvOffset);
        GLint resLoc = glGetUniformLocation(program_->GetId(), "u_Resolution");
        glUniform2f(resLoc, viewport_.x, viewport_.y);
            //(float)Game::GameCore::RETRO_LAYOUT.width,
            //(float)Game::GameCore::RETRO_LAYOUT.height);
            //textureSize_.y);
        GLint texSizeLoc = glGetUniformLocation(program_->GetId(), "u_TexSize");
        glUniform2f(texSizeLoc, textureSize_.x, textureSize_.y);
        
        // 設定渲染狀態
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);
        vertexArray_->Bind();
        vertexArray_->DrawTriangles();
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }
}
