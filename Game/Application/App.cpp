#include "pch.hpp"
#include "App.h"

#include "States/MenuState.h"
#include "States/GameplayState.h"
#include "GameCore/RetroLayout.h"
#include "Util/Input.hpp"


namespace Game::Application
{                        
    void App::Start()
    {
        // 創建 FBO
        {
            glGenFramebuffers(1, &m_Fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, m_Fbo);

            glGenTextures(1, &m_ColorTex);
            glBindTexture(GL_TEXTURE_2D, m_ColorTex);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                Game::GameCore::RETRO_LAYOUT.width, Game::GameCore::RETRO_LAYOUT.height,
                0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
            glFramebufferTexture2D(GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorTex, 0);

            glGenRenderbuffers(1, &m_DepthRbo);
            glBindRenderbuffer(GL_RENDERBUFFER, m_DepthRbo);
            glRenderbufferStorage(
                GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
                Game::GameCore::RETRO_LAYOUT.width, Game::GameCore::RETRO_LAYOUT.height);
            glFramebufferRenderbuffer(
                GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthRbo);
        }

        // 初始化狀態機
        {
            ChangeState(std::make_unique<Game::States::MenuState>());
            m_CurrentState = State::UPDATE;
        }
    }
    void App::Update()
    {
        if (Util::Input::IfExit() || Util::Input::IsKeyUp(Util::Keycode::ESCAPE))
        {
            m_CurrentState = State::END;
            return;
        }

        // 清空畫布
        {
            glBindFramebuffer(GL_FRAMEBUFFER, m_Fbo);
            glViewport(0, 0, Game::GameCore::RETRO_LAYOUT.width, Game::GameCore::RETRO_LAYOUT.height);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        // 更新並渲染狀態機
        {
            static uint32_t lastTime = SDL_GetTicks();
            uint32_t currentTime = SDL_GetTicks();
            float dt = (currentTime - lastTime) / 1000.0f;
            lastTime = currentTime;
            if (dt > 0.05f) dt = 0.05f;

            if (m_ActiveState) m_ActiveState->Update(this, dt);
            if (m_ActiveState) m_ActiveState->Render(this);
        }

        // 放大 FBO 並顯示
        {
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glViewport(0, 0,
                Game::GameCore::RETRO_LAYOUT.width * Game::GameCore::RETRO_LAYOUT.pixelScale,
                Game::GameCore::RETRO_LAYOUT.height * Game::GameCore::RETRO_LAYOUT.pixelScale);
            glBlitFramebuffer(
                0, 0,
                Game::GameCore::RETRO_LAYOUT.width,
                Game::GameCore::RETRO_LAYOUT.height,
                0, 0,
                Game::GameCore::RETRO_LAYOUT.width * Game::GameCore::RETRO_LAYOUT.pixelScale,
                Game::GameCore::RETRO_LAYOUT.height * Game::GameCore::RETRO_LAYOUT.pixelScale,
                GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }
    }
    void App::ChangeState(std::unique_ptr<Game::States::GameState> newState)
    {
        if (m_ActiveState) m_ActiveState->End(this);
        m_ActiveState = std::move(newState);
        if (m_ActiveState) m_ActiveState->Start(this);
    }
    void App::End() 
    {
        if (m_ActiveState) m_ActiveState->End(this);
    }
}
