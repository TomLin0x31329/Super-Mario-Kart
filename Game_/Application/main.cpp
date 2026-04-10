#include "pch.hpp"
#include "Core/Context.hpp"

#include "Application/App.h"
#include "GameCore/RetroLayout.h"


int main(int argc, char* argv[]) 
{
    Game::Application::App app;
    auto context = Core::Context::GetInstance(
        Game::GameCore::RETRO_LAYOUT.width * Game::GameCore::RETRO_LAYOUT.pixelScale,
        Game::GameCore::RETRO_LAYOUT.height * Game::GameCore::RETRO_LAYOUT.pixelScale);

    // set icon in window.
    context->SetWindowIcon(RESOURCE_DIR "/icon.png");

    while (!context->GetExit()) 
    {
        context->Setup();

        switch (app.GetCurrentState()) 
        {
        case  Game::Application::App::State::START:
            app.Start();
            break;

        case  Game::Application::App::State::UPDATE:
            app.Update();
            break;

        case  Game::Application::App::State::END:
            app.End();
            context->SetExit(true);
            break;
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        context->Update();
    }

    return 0;
}
