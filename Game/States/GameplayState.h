#pragma once

#include "pch.hpp"
#include "Util/Renderer.hpp"
#include "Util/Image.hpp"
#include "Util/BGM.hpp"
#include "Util/SFX.hpp"

#include "GameState.h"
#include "Renderers/ScrollingUIRenderer.h"
#include "Renderers/UIRenderer.h"
#include "Renderers/BakedSurfaceRenderer.h"
#include "Renderers/BillboardRenderer.h"
#include "Systems/LevelLoader.h"
#include "Gameplay/PlayerKart.h"
#include "Gameplay/GameObject.h"

namespace Game::States
{
	enum class GameplaySubState
	{
		Begin,
		AnimLakitu1,
		AnimLakitu2,
		AnimLakituLight1,
		AnimLakituLight2,
		AnimLakituLight3,
		AnimLakituLight4,
		AnimLakitu3,
		Game,
		AnimFinish,
		ShowScore,
		AnimToMenu,
		End
	};
	class GameplayState : public GameState
	{
	private:
		std::unique_ptr<Game::Renderer::UIRenderer> m_UIRenderer;
		std::unique_ptr<Game::Renderer::ScrollingUIRenderer> m_ScrollingUIRenderer;
		std::unique_ptr<Game::Renderer::BakedSurfaceRenderer> m_BakedRenderer;
		std::unique_ptr<Game::Renderer::BillboardRenderer> m_BillboardRenderer;

		Util::Renderer m_Root;
		int m_DriverIndex = 0;
		std::unique_ptr<Game::Gameplay::GameObject> m_PlayerCollider;
		std::vector<Game::Gameplay::PlayerKart> m_Drivers;
		std::vector<Game::Gameplay::GameObject> m_LevelObjects;
		glm::vec2 m_CameraPos = { 0.0f, 0.0f };
		float m_CameraYaw = 0.0f;

		// Background 捲動背景參數
		float m_SkyboxOffset = 0.0f;
		float m_SkyboxAutoScrollSpeed = 1.6f;
		float m_TreeOffset = 0.0f;
		float m_TreeAutoScrollSpeed = 0.16f;

		// Lakitu 狀態機計時器
		float m_StateTimer = 0.0f;
		const float m_TransitionDuration = 0.2f;
		glm::vec2 m_LakituPos = { 28.0f, -32.0f };

		// Lakitu Lap 過圈動畫
		bool m_LapAnimActive = false;
		float m_LapAnimTimer = 0.0f;
		glm::vec2 m_LapAnimPos = { 0.0f, 0.0f };
		int m_CurrentAnimLap = 0;

		float m_TargetCameraYaw = 0.0f;

		// 計數器
		uint32_t m_RaceStartTime = 0;
		uint32_t m_RaceCurrentTime = 0;
		uint32_t m_coinCount = 0;
		int m_LapCount = 1;               // 當前圈數 (1 表示第一圈)
		int m_NextRequiredCheckpoint = 1; // 起跑後，必須先踩到 ID 1 的起點線才算正式開始

		uint32_t m_CurrentLapStartTime = 0;  // 記錄「當前這圈」是何時開始的
		std::vector<uint32_t> m_LapTimes;    // 儲存每一圈的耗時 (毫秒)

		Game::GameCore::LevelData m_LevelData;
		Game::GameCore::ObjectDefinition m_PipeDefinition;
		Game::GameCore::ObjectDefinition m_MapMarkerDef;

		// Menu 狀態機
		GameplaySubState m_CurrentSubState = GameplaySubState::Begin;

		std::shared_ptr<Util::Image> m_BlackImage;
		std::shared_ptr<Util::Image> m_SkyboxImage;
		std::shared_ptr<Util::Image> m_TreeImage;

		std::shared_ptr<Util::Image> m_LakituLight[4];
		std::shared_ptr<Util::Image> m_LakituLap[4];
		std::shared_ptr<Util::Image> m_LakituFlag[3];

		std::shared_ptr<Util::Image> m_UiItemSlotIcon;
		std::shared_ptr<Util::Image> m_UiTimeNumberIcon[11];
		std::shared_ptr<Util::Image> m_UiScoreKartIcon;
		std::shared_ptr<Util::Image> m_UiScoreCoinIcon;
		std::shared_ptr<Util::Image> m_UiScoreNumberIcon[11];
		std::shared_ptr<Util::Image> m_UiScoreRedNumberIcon[10];

		std::shared_ptr<Util::Image> m_UiLapTimeScoreboardIcon;

		std::unique_ptr<Util::BGM> m_MarioCircuitBGM;
		std::unique_ptr<Util::BGM> m_MarioCircuitFinalLapBGM;
		std::unique_ptr<Util::BGM> m_MarioCircuitRanksBGM;

		std::unique_ptr<Util::SFX> m_RaceStartSFX;
		std::unique_ptr<Util::SFX> m_ThudCoinSFX;
		std::unique_ptr<Util::SFX> m_ThudPipeSFX;
		std::unique_ptr<Util::SFX> m_MarioCircuitFinalLapNoticeSFX;
		std::unique_ptr<Util::SFX> m_MarioCircuitNewRecordSFX;

	private:
		void ResetScene();
		void RenderScene();
		void UpdateKart(float dt);
		void UpdateCameraAndBackground();
		void UpdateKartSpriteRotation();

		void ApplyFade(float alpha);

	public:
		GameplayState(int driverIndex);

	public:
		void Start(Game::Application::App* app) override;
		void Update(Game::Application::App* app, float dt) override;
		void Render(Game::Application::App* app) override;
		void End(Game::Application::App* app) override;

	};
}
