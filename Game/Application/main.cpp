#include "pch.hpp"
#include "Core/Context.hpp"

#include "Application/App.h"
#include "GameCore/RetroLayout.h"


int main(int argc, char* argv[]) 
{
    auto context = Core::Context::GetInstance(
        Game::GameCore::RETRO_LAYOUT.width * Game::GameCore::RETRO_LAYOUT.pixelScale,
        Game::GameCore::RETRO_LAYOUT.height * Game::GameCore::RETRO_LAYOUT.pixelScale);
    context->SetWindowIcon(RESOURCE_DIR "/icon.png");

    Game::Application::App app;

    while (!context->GetExit()) 
    {
        context->Setup();

        switch (app.GetCurrentState()) 
        {
        case  Game::Application::State::START:
            app.Start();
            break;

        case  Game::Application::State::UPDATE:
            app.Update();
            break;

        case  Game::Application::State::END:
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
