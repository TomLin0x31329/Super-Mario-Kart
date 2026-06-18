#pragma once

namespace Game::Application { class App; }
namespace Game::States
{
    class GameState
    {
    public:
        virtual ~GameState() = default;

    public:
        virtual void Start(Game::Application::App* app) = 0;
        virtual void Update(Game::Application::App* app, float dt) = 0;
        virtual void Render(Game::Application::App* app) = 0;
        virtual void End(Game::Application::App* app) = 0;

    };
}
