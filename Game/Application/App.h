#pragma once

#include "States/GameState.h"

#include <memory>

namespace Game::Application
{
    enum class State { START, UPDATE, END };
    class App
    {
    public:
        App() = default;
        ~App() = default;

    private:
        std::unique_ptr<Game::States::GameState> m_ActiveState;

    public:
        unsigned int m_Fbo = 0;
        unsigned int m_ColorTex = 0;
        unsigned int m_DepthRbo = 0;
        State m_CurrentState = State::START;

    public:
        void Start();
        void Update();
        void End();

        State GetCurrentState() const { return m_CurrentState; }
        void ChangeState(std::unique_ptr<Game::States::GameState> newState);

    };
}
