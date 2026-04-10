#pragma once

#include <GL/glew.h>
#include "pch.hpp" // IWYU pragma: export
#include "Util/Renderer.hpp"

#include "GameCore/LevelData.h"
#include "GameCore/ObjectDefinition.h"
#include "GameCore/RetroLayout.h"

#include "Renderers/BackgroundRenderer.h"
#include "Renderers/BakedSurfaceRenderer.h"
#include "Renderers/BillboardRenderer.h"
#include "Renderers/UIRenderer.h"

#include "Systems/LevelLoader.h"

namespace Game::Application
{
	class App
	{
	public:
		enum class State { START, UPDATE, END, };

	private:
		State m_CurrentState = State::START;

		GLuint m_Fbo = 0;
		GLuint m_ColorTex = 0;
		GLuint m_DepthRbo = 0;

		Util::Renderer m_Root;

		std::unique_ptr<Game::Renderer::BackgroundRenderer> m_SkyboxRenderer;
		std::unique_ptr<Game::Renderer::BackgroundRenderer> m_TreesRenderer;
		std::unique_ptr<Game::Renderer::BakedSurfaceRenderer> m_BakedRenderer;
		std::unique_ptr<Game::Renderer::BillboardRenderer> m_BillboardRenderer;
		std::unique_ptr<Game::Renderer::UIRenderer> m_UIRenderer;

		glm::vec2 m_CameraPos = { 0.0f, 0.0f };
		float m_CameraYaw = 0.0f;

		Game::GameCore::LevelData levelData_;
		Game::GameCore::ObjectDefinition m_PipeDefinition;

		std::shared_ptr<Util::Image> uiItemSlotIcon_;
		std::shared_ptr<Util::Image> uiTimeNumberIcon_[11];

		std::shared_ptr<Util::Image> uiScoreKartIcon_;
		std::shared_ptr<Util::Image> uiScoreCoinIcon_;
		std::shared_ptr<Util::Image> uiScoreNumberIcon_[11];

	public:
		void Start();
		void Update();
		void End();

		State GetCurrentState() const { return m_CurrentState; }

	};
}
