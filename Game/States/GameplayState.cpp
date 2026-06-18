#include "pch.hpp"

#include "GameplayState.h"
#include "MenuState.h"

#include "Application/App.h"
#include "Util/Input.hpp"
#include "Util/Logger.hpp"

#include <memory>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


namespace
{
	float GetShortestAngle(float target, float current)
	{
		float diff = std::fmod(target - current, 2.0f * (float)M_PI);
		if (diff > (float)M_PI) diff -= 2.0f * (float)M_PI;
		if (diff < -(float)M_PI) diff += 2.0f * (float)M_PI;
		return diff;
	}
}
namespace Game::States
{
	void GameplayState::ResetScene()
	{
		// 初始化 Track
		{
			SDL_Surface* coinSurface = IMG_Load(RESOURCE_DIR "/sprites/items/coin.png");
			if (coinSurface)
			{
				m_BakedRenderer->BakeObject(coinSurface, m_LevelData.coins);
				SDL_FreeSurface(coinSurface);
				LOG_INFO("Successfully baked {} coins onto the track.",
					m_LevelData.coins.size());
			}
			else
			{
				LOG_ERROR("Failed to load coin sprite for baking: {}", IMG_GetError());
			}
		}

		// 初始化 Driver
		{
			m_Drivers[m_DriverIndex].Position = m_LevelData.startingGrid[0].pos * 1.6f;
			m_Drivers[m_DriverIndex].ExpectedDir = { 0.0f, -1.0f };
			m_Drivers[m_DriverIndex].Velocity = { 0.0f, -0.01f };
		}

		// 初始化 Camera
		{
			m_TargetCameraYaw = 0.0f;
			UpdateCameraAndBackground();
		}

		// 初始化 Coin
		{
			for (auto& obj : m_LevelObjects)
			{
				obj.Activate();
			}
		}

		// 初始化計數器
		{
			m_StateTimer = 0.0f;
			m_coinCount = 0;
			m_LakituPos = { 28.0f, -32.0f };

			m_LapCount = 1;
			m_NextRequiredCheckpoint = 1;

			m_LapTimes.clear();
			m_CurrentLapStartTime = 0;
		}

		UpdateKart(0.0f);
	}
	void GameplayState::RenderScene()
	{
		const auto& layout = Game::GameCore::RETRO_LAYOUT;

		// 渲染場景
		{
			glEnable(GL_SCISSOR_TEST);

			// 繪製上半部視窗
			{
				glScissor(0, layout.GetTrackY(), layout.width, layout.height - layout.GetTrackY());

				// 繪製背景
				glViewport(0, 0, layout.width, layout.height);
				m_ScrollingUIRenderer->Draw(
					m_SkyboxImage,
					0.0f, layout.topBar,
					768.0f, 21.0f, m_SkyboxOffset, 0.0f);
				m_ScrollingUIRenderer->Draw(
					m_TreeImage,
					0.0f, layout.topBar,
					1280.0f, 21.0f, m_TreeOffset, 0.0f);

				// 繪製賽道
				glViewport(0, layout.GetTrackY(), layout.width, layout.track);
				m_BakedRenderer->Draw(m_CameraPos, m_CameraYaw, 0.04f, 0.42f, 120.0f);

				// 繪製水管
				glViewport(0, 0, layout.width, layout.height);
				std::vector<Game::Renderer::BillboardItem> renderItems;
				for (const auto& pipePos : m_LevelData.pipes)
				{
					Game::Renderer::BillboardItem item;
					item.worldPos.x = pipePos.x * 1.6f;
					item.worldPos.y = pipePos.y * 1.6f;
					item.yaw = 0.0f;
					item.definition = &m_PipeDefinition;

					renderItems.push_back(item);
				}
				m_BillboardRenderer->Draw(
					m_CameraPos, m_CameraYaw,
					0.04f, 0.42f, 120.0f,
					layout.GetTrackY(), layout.track, renderItems);
			}

			// 繪製下半部視窗
			{
				glScissor(0, layout.bottomBar, layout.width, layout.map);

				// 繪製地圖
				glViewport(0, layout.bottomBar, layout.width, layout.map);
				m_BakedRenderer->Draw({ 820.0f, 5920.0f }, 0.0f, 2.1f, 0.2f, 13200.0f);

				// 恢復 Viewport
				glViewport(0, 0, layout.width, layout.height);

				// 繪製 Driver 標示
				std::shared_ptr<Util::Image> mapSprite;
				bool mapFlip = false;
				m_Drivers[m_DriverIndex].GetSpriteForCamera(0.0f, mapSprite, mapFlip, 2);
				if (!m_MapMarkerDef.lods.empty() && !m_MapMarkerDef.lods[0].sprites.empty())
				{
					m_MapMarkerDef.lods[0].sprites[0].image = mapSprite;
				}
				Game::Renderer::BillboardItem item;
				item.worldPos = m_Drivers[m_DriverIndex].Position;
				item.worldPos.y += 100.0f;
				item.yaw = 0.0f;
				item.definition = &m_MapMarkerDef;
				item.shouldFlip = mapFlip;
				m_BillboardRenderer->Draw(
					{ 820.0f, 5920.0f }, 0.0f,
					2.1f, 0.2f, 13200.0f,
					layout.bottomBar, layout.map,
					{ item });
			}

			glDisable(GL_SCISSOR_TEST);
		}

		// 渲染 UI
		if (m_CurrentSubState != GameplaySubState::ShowScore)
		{
			// Timer
			uint32_t currentTimeMs = m_RaceCurrentTime - m_RaceStartTime;
			uint32_t minutes = (currentTimeMs / 60000) % 100;
			uint32_t seconds = (currentTimeMs / 1000) % 60;
			uint32_t centiseconds = (currentTimeMs / 10) % 100;
			int m10 = minutes / 10;
			int m1 = minutes % 10;
			int s10 = seconds / 10;
			int s1 = seconds % 10;
			int ms10 = centiseconds / 10;
			int ms1 = centiseconds % 10;
			m_UIRenderer->DrawImage(m_UiItemSlotIcon, 147.0f, 5.0f);
			m_UIRenderer->DrawImage(m_UiTimeNumberIcon[m10], 176.0f, 7.0f);
			m_UIRenderer->DrawImage(m_UiTimeNumberIcon[m1], 184.0f, 7.0f);
			m_UIRenderer->DrawImage(m_UiTimeNumberIcon[10], 192.0f, 7.0f);
			m_UIRenderer->DrawImage(m_UiTimeNumberIcon[s10], 200.0f, 7.0f);
			m_UIRenderer->DrawImage(m_UiTimeNumberIcon[s1], 208.0f, 7.0f);
			m_UIRenderer->DrawImage(m_UiTimeNumberIcon[10], 216.0f, 7.0f);
			m_UIRenderer->DrawImage(m_UiTimeNumberIcon[10], 219.0f, 7.0f);
			m_UIRenderer->DrawImage(m_UiTimeNumberIcon[ms10], 224.0f, 7.0f);
			m_UIRenderer->DrawImage(m_UiTimeNumberIcon[ms1], 232.0f, 7.0f);

			// Coin
			m_UIRenderer->DrawImage(m_UiScoreCoinIcon, 190.0f, 92.0f);
			m_UIRenderer->DrawImage(m_UiScoreNumberIcon[10], 198.0f, 93.0f);
			m_UIRenderer->DrawImage(m_UiScoreNumberIcon[m_coinCount / 10], 206.0f, 93.0f);
			m_UIRenderer->DrawImage(m_UiScoreNumberIcon[m_coinCount % 10], 214.0f, 93.0f);

			m_UIRenderer->Render();
		}

		// 渲染 PlayerKart
		{
			auto kartImage = m_Drivers[m_DriverIndex].m_CurrentSprite;
			if (kartImage)
			{
				float kartDrawX = 128.0f - (kartImage->GetSize().x * 0.5f);
				float kartDrawY = 80.0f - (kartImage->GetSize().y * 0.5f);

				m_UIRenderer->DrawImage(
					kartImage, kartDrawX, kartDrawY, m_Drivers[m_DriverIndex].m_ShouldFlipSprite);
			}
			m_UIRenderer->Render();
		}
	}
	void GameplayState::UpdateKart(float dt)
	{
		m_RaceCurrentTime = SDL_GetTicks();

		// 更新卡丁車物理
		{
			m_Drivers[m_DriverIndex].Update(dt, m_LevelData.collisionMap, m_CameraYaw);
			m_PlayerCollider->SetPosition(m_Drivers[m_DriverIndex].Position);
			for (auto& obj : m_LevelObjects)
			{
				if (obj.IsActive())
				{
					m_PlayerCollider->UpdateCollision(obj);
				}
			}

			if (m_Drivers[m_DriverIndex].IsHitWall())
			{
				m_ThudPipeSFX->Play();
			}

			// 檢查點碰撞偵測
			glm::vec2 playerPos = m_Drivers[m_DriverIndex].Position;
			playerPos.x /= 1.6f;
			playerPos.y /= 1.6f;
			for (const auto& cp : m_LevelData.checkpoints)
			{
				bool isInside = (
					playerPos.x >= cp.min.x && playerPos.x <= cp.max.x &&
					playerPos.y >= cp.min.y && playerPos.y <= cp.max.y);
				if (isInside && cp.id == m_NextRequiredCheckpoint)
				{
					if (cp.id == 1 && (m_RaceCurrentTime - m_RaceStartTime) > 10000)
					{
						// 結算單圈時間
						{
							m_LapTimes.push_back(m_RaceCurrentTime - m_CurrentLapStartTime);
							m_CurrentLapStartTime = m_RaceCurrentTime;

							// 將毫秒轉換為 分:秒:毫秒 格式顯示在 Log 中
							uint32_t mins = (m_LapTimes.back() / 60000) % 100;
							uint32_t secs = (m_LapTimes.back() / 1000) % 60;
							uint32_t ms = (m_LapTimes.back() / 10) % 100;
							LOG_INFO(" Lap {} Time: {:02d}:{:02d}.{:02d}", m_LapCount, mins, secs, ms);
						}

						m_NextRequiredCheckpoint = 2;
						m_LapCount++;
						LOG_INFO("LAP COMPLETED! Current Lap: {} / {}", m_LapCount, m_LevelData.totalLaps);

						if (m_LapCount >= 2 && m_LapCount - 2 < 5)
						{
							m_LapAnimActive = true;
							m_LapAnimTimer = 0.0f;
							m_CurrentAnimLap = m_LapCount;
						}

						if (m_LapCount == m_LevelData.totalLaps)
						{
							m_MarioCircuitBGM->FadeOut(200);
							m_MarioCircuitFinalLapNoticeSFX->Play();
						}
						if (m_LapCount > m_LevelData.totalLaps)
						{
							m_MarioCircuitFinalLapBGM->FadeOut(1000);
							m_MarioCircuitNewRecordSFX->Play();

							LOG_INFO("RACE FINISHED!");

							m_CurrentSubState = GameplaySubState::AnimFinish;
							m_StateTimer = 0.0f;
							m_LakituPos.y = -40.0f;
							m_LakituPos.x = 48.0f;

							float velocityAngle = std::atan2(
								m_Drivers[m_DriverIndex].Velocity.x,
								-m_Drivers[m_DriverIndex].Velocity.y);
							m_TargetCameraYaw = velocityAngle + (float)M_PI;
						}
					}
					else if (cp.id == 2)
					{
						m_NextRequiredCheckpoint = 1;
					}
				}
			}
		}

		// 更新 Camera
		{
			if (glm::length(m_Drivers[m_DriverIndex].Velocity) > 5.0f)
			{
				float expectedAngle = std::atan2(
					m_Drivers[m_DriverIndex].ExpectedDir.x,
					-m_Drivers[m_DriverIndex].ExpectedDir.y);

				float angleDiff = GetShortestAngle(expectedAngle, m_CameraYaw);
				m_CameraYaw += angleDiff * 0.15f;
			}
			UpdateCameraAndBackground();
		}

		// 更新 Lakitu Lap anime
		if (m_LapAnimActive)
		{
			m_LapAnimTimer += dt;

			auto img = m_LakituLap[m_CurrentAnimLap - 2];
			float imgW = img->GetSize().x;
			float imgH = img->GetSize().y;

			// 定義動畫階段時間
			const float phase1Duration = 1.2f; // 進場到中間並停留的時間
			const float phase2Duration = 0.6f; // 往上飛走的時間

			if (m_LapAnimTimer <= phase1Duration)
			{
				float t = m_LapAnimTimer / phase1Duration;
				float easeOut = 1.0f - std::pow(1.0f - t, 3.0f);

				glm::vec2 startPos = { -imgW, -imgH };
				glm::vec2 endPos = { 64.0f, 48.0f };

				m_LapAnimPos = startPos + (endPos - startPos) * easeOut;
			}
			else if (m_LapAnimTimer <= phase1Duration + phase2Duration)
			{
				float t = (m_LapAnimTimer - phase1Duration) / phase2Duration;
				float easeIn = std::pow(t, 3.0f);

				glm::vec2 startPos = { 64.0f, 48.0f };
				glm::vec2 endPos = { startPos.x, -imgH - 20.0f };

				m_LapAnimPos = startPos + (endPos - startPos) * easeIn;
			}
			else
			{
				m_LapAnimActive = false;
				if (m_LapCount == 5)
				{
					m_MarioCircuitFinalLapBGM->FadeIn(200, -1);

				}
			}
		}
	}
	void GameplayState::UpdateCameraAndBackground()
	{
		glm::vec2 cameraForward = { std::sin(m_CameraYaw), -std::cos(m_CameraYaw) };
		float cameraDistance = 150.0f;

		m_CameraPos = m_Drivers[m_DriverIndex].Position - cameraForward * cameraDistance;

		float turns = m_CameraYaw / (2.0f * (float)M_PI);
		m_SkyboxOffset = turns * 0.5f;
		m_TreeOffset = m_SkyboxOffset * 1.666666667f;
	}
	void GameplayState::UpdateKartSpriteRotation()
	{
		float kartYaw = std::atan2(
			m_Drivers[m_DriverIndex].Velocity.x,
			-m_Drivers[m_DriverIndex].Velocity.y);

		float relAngle = GetShortestAngle(kartYaw, m_CameraYaw);
		if (relAngle < 0) relAngle += 2.0f * (float)M_PI;

		int frameIndex = (int)(relAngle / (2.0f * (float)M_PI) * 22.0f + 0.5f) % 22;

		int spriteIndex = 0;
		bool flip = false;

		if (frameIndex <= 11)
		{
			spriteIndex = frameIndex;
			flip = false;
		}
		else
		{
			spriteIndex = 22 - frameIndex;
			flip = true;
		}

		m_Drivers[m_DriverIndex].m_CurrentSprite =
			m_Drivers[m_DriverIndex].m_SpritesLod0[spriteIndex];
		m_Drivers[m_DriverIndex].m_ShouldFlipSprite = flip;
	}

	void GameplayState::ApplyFade(float alpha)
	{
		if (alpha > 0.0f)
		{
			const auto& layout = Game::GameCore::RETRO_LAYOUT;
			glDisable(GL_SCISSOR_TEST);
			glViewport(0, 0, layout.width, layout.height);

			m_ScrollingUIRenderer->Draw(
				m_BlackImage,
				0.0f, 0.0f,
				layout.width, layout.height,
				0.0f, 0.0f,
				false, false,
				alpha
			);
		}
	}
}
namespace Game::States
{
	GameplayState::GameplayState(int driverIndex)
		:m_DriverIndex(driverIndex)
	{

	}
}
namespace Game::States
{
	void GameplayState::Start(Game::Application::App* app)
	{
		// 初始化 Renderer
		{
			m_UIRenderer = std::make_unique<Game::Renderer::UIRenderer>();
			m_UIRenderer->Init();
			m_ScrollingUIRenderer = std::make_unique<Game::Renderer::ScrollingUIRenderer>();
			m_ScrollingUIRenderer->Init();
			m_BakedRenderer = std::make_unique<Game::Renderer::BakedSurfaceRenderer>();
			m_BakedRenderer->Init(
				RESOURCE_DIR "/sprites/tracks/level_1/track.png",
				RESOURCE_DIR "/sprites/tracks/level_1/out_of_bounds.png");
			m_BillboardRenderer = std::make_unique<Game::Renderer::BillboardRenderer>();
			m_BillboardRenderer->Init();
		}

		// 設置 Pipe LOD
		{
			m_PipeDefinition.collisionRadius = 6.0f;
			float lodDistances[12] = {
				120.0f,
				160.0f,
				200.0f,
				240.0f,
				280.0f,
				320.0f,
				380.0f,
				440.0f,
				600.0f,
				800.0f,
				2000.0f,
				2400.0f
			};
			for (int i = 0; i < 12; ++i)
			{
				Game::GameCore::SpriteLOD lod;
				lod.maxDistance = lodDistances[i];

				Game::GameCore::DirectionalSprite sprite;
				sprite.angle = 0.0f;

				std::string path = std::string(RESOURCE_DIR) + "/sprites/items/pipe/" + std::to_string(i + 1) + ".png";
				sprite.image = std::make_shared<Util::Image>(path);

				lod.sprites.push_back(sprite);
				m_PipeDefinition.lods.push_back(lod);
			}
		}

		// 加載 Level
		{
			m_LevelData = Game::Systems::LevelLoader::LoadLevel(RESOURCE_DIR "/Levels/Level1.json");
			m_LevelObjects.clear();

			// 初始化金幣
			Game::GameCore::ObjectDefinition coinDef;
			coinDef.collisionRadius = 6.0f;
			for (const auto& pos : m_LevelData.coins)
			{
				m_LevelObjects.emplace_back(
					pos * 1.6f,
					coinDef,
					Game::Gameplay::ObjectType::Coin
				);
			}
			// 初始化水管
			for (const auto& pos : m_LevelData.pipes)
			{
				m_LevelObjects.emplace_back(
					pos * 1.6f,
					m_PipeDefinition,
					Game::Gameplay::ObjectType::Pipe
				);
			}
		}

		// 加載 Drivers
		{
			m_Drivers.resize(8);
			m_Drivers[0].Init("mario_kart");
			m_Drivers[1].Init("peach_kart");

			Game::GameCore::ObjectDefinition playerDef;
			playerDef.collisionRadius = 4.0f;
			m_PlayerCollider = std::make_unique<Game::Gameplay::GameObject>(
				m_Drivers[m_DriverIndex].Position,
				playerDef,
				Game::Gameplay::ObjectType::Player,
				[this](Game::Gameplay::GameObject* self,
					Game::Gameplay::GameObject* other) {
						if (other->GetType() == Game::Gameplay::ObjectType::Coin)
						{
							m_ThudCoinSFX->Play();

							LOG_INFO("Hit coin");
							m_coinCount++;
							other->Deactivate();
							m_BakedRenderer->RestoreBakedObject({ 7, 7 }, { other->GetPosition() / 1.6f });
						}
						else if (other->GetType() == Game::Gameplay::ObjectType::Pipe)
						{
							m_ThudPipeSFX->Play();

							LOG_INFO("Hit pipe");

							// 1. 取得兩者的圓心座標
							glm::vec2 playerPos = self->GetPosition();
							glm::vec2 pipePos = other->GetPosition();

							glm::vec2 diff = playerPos - pipePos;
							float dist = glm::length(diff);

							// 發生碰撞
							if (dist > 0.0f)
							{
								glm::vec2 normal = diff / dist;

								float combinedRadius = 10.0f;
								float overlap = combinedRadius - dist;
								if (overlap > 0.0f)
								{
									m_Drivers[m_DriverIndex].Position += normal * (overlap + 0.4f);
									self->SetPosition(m_Drivers[m_DriverIndex].Position);
								}

								// 處理反彈
								glm::vec2 currentVel = m_Drivers[m_DriverIndex].Velocity;
								if (glm::dot(currentVel, normal) < 0.0f)
								{
									glm::vec2 reflectVel = currentVel - 2.0f * glm::dot(currentVel, normal) * normal;
									float bounciness = 0.6f;
									glm::vec2 finalBounceVel = reflectVel * bounciness;
									if (glm::length(finalBounceVel) < 60.0f)
									{
										finalBounceVel = normal * 60.0f;
									}

									m_Drivers[m_DriverIndex].Velocity = finalBounceVel;
								}
							}
						}
				}
			);
		}

		// 加載 UI Image
		{
			// Fade black image
			m_BlackImage = std::make_shared<Util::Image>(
				RESOURCE_DIR "/sprites/ui/menu/black.png");

			// Background
			m_SkyboxImage = std::make_shared<Util::Image>(
				RESOURCE_DIR "/sprites/tracks/level_1/sky.png");
			m_TreeImage = std::make_shared<Util::Image>(
				RESOURCE_DIR "/sprites/tracks/level_1/trees.png");

			// Lakitu
			for (int i = 0; i < 4; i++)
			{
				m_LakituLight[i] = std::make_shared<Util::Image>(
					RESOURCE_DIR "/sprites/Lakitu/Light" + std::to_string(i + 1) + ".png");
				m_LakituLap[i] = std::make_shared<Util::Image>(
					RESOURCE_DIR "/sprites/Lakitu/Lap" + std::to_string(i + 2) + ".png");
			}
			for (int i = 0; i < 3; i++)
			{
				m_LakituFlag[i] = std::make_shared<Util::Image>(
					RESOURCE_DIR "/sprites/Lakitu/Flag" + std::to_string(i + 1) + ".png");
			}

			// Timer
			m_UiItemSlotIcon = std::make_shared<Util::Image>(
				RESOURCE_DIR "/sprites/ui/item_slot.png");
			for (int i = 0; i < 10; ++i)
			{
				m_UiTimeNumberIcon[i] = std::make_shared<Util::Image>(
					RESOURCE_DIR "/sprites/ui/time/number/" + std::to_string(i) + ".png");
			}
			m_UiTimeNumberIcon[10] = std::make_shared<Util::Image>(
				RESOURCE_DIR "/sprites/ui/time/number/comma.png");

			// Score
			m_UiScoreKartIcon = std::make_shared<Util::Image>(
				RESOURCE_DIR "/sprites/ui/score/kart.png");
			m_UiScoreCoinIcon = std::make_shared<Util::Image>(
				RESOURCE_DIR "/sprites/ui/score/coin.png");
			for (int i = 0; i < 10; ++i)
			{
				m_UiScoreNumberIcon[i] = std::make_shared<Util::Image>(
					RESOURCE_DIR "/sprites/ui/score/number/" + std::to_string(i) + ".png");
				m_UiScoreRedNumberIcon[i] = std::make_shared<Util::Image>(
					RESOURCE_DIR "/sprites/ui/score/red_number/" + std::to_string(i) + ".png");
			}
			m_UiScoreNumberIcon[10] = std::make_shared<Util::Image>(
				RESOURCE_DIR "/sprites/ui/score/number/multiply.png");

			// Scoreboard
			m_UiLapTimeScoreboardIcon = std::make_shared<Util::Image>(
				RESOURCE_DIR "/sprites/ui/time/lap_time.png");

			// Mini map
			m_MapMarkerDef.collisionRadius = 0.0f;
			Game::GameCore::SpriteLOD markerLod;
			markerLod.maxDistance = 999999.0f;

			Game::GameCore::DirectionalSprite markerSprite;
			markerSprite.angle = 0.0f;
			markerSprite.image = m_UiScoreKartIcon;	// TODO: 旋轉的小地圖提示

			markerLod.sprites.push_back(markerSprite);
			m_MapMarkerDef.lods.push_back(markerLod);
		}

		m_MarioCircuitBGM = std::make_unique<Util::BGM>(RESOURCE_DIR "/audio/bgm/1-05. Mario Circuit.mp3");
		m_MarioCircuitBGM->SetVolume(64);
		m_MarioCircuitFinalLapBGM = std::make_unique<Util::BGM>(RESOURCE_DIR "/audio/bgm/1-30. Mario Circuit (Final Lap).mp3");
		m_MarioCircuitFinalLapBGM->SetVolume(64);
		m_MarioCircuitRanksBGM = std::make_unique<Util::BGM>(RESOURCE_DIR "/audio/bgm/2-12. Mario's Ranks (Prototype 1992-04-13).mp3");
		m_MarioCircuitRanksBGM->SetVolume(64);

		m_RaceStartSFX = std::make_unique<Util::SFX>(RESOURCE_DIR "/audio/sfx/racestart.wav");
		m_RaceStartSFX->SetVolume(48);
		m_ThudCoinSFX = std::make_unique<Util::SFX>(RESOURCE_DIR "/audio/sfx/coin.wav");
		m_ThudCoinSFX->SetVolume(64);
		m_ThudPipeSFX = std::make_unique<Util::SFX>(RESOURCE_DIR "/audio/sfx/thudpipe.wav");
		m_ThudPipeSFX->SetVolume(64);
		m_MarioCircuitFinalLapNoticeSFX = std::make_unique<Util::SFX>(RESOURCE_DIR "/audio/bgm/2-07. Final Lap Notice (Prototype 1992-04-13).mp3");
		m_MarioCircuitFinalLapNoticeSFX->SetVolume(64);
		m_MarioCircuitNewRecordSFX = std::make_unique<Util::SFX>(RESOURCE_DIR "/audio/bgm/2-09. New Record (Prototype 1992-04-13).mp3");
		m_MarioCircuitNewRecordSFX->SetVolume(72);

		LOG_INFO("GameplayState::Start Finished.");
		ResetScene();
	}
	void GameplayState::Update(Game::Application::App* app, float dt)
	{
		// 切換狀態機
		switch (m_CurrentSubState)
		{
		case GameplaySubState::Begin:
		{
			m_RaceCurrentTime = m_RaceStartTime;

			m_StateTimer += dt;
			if (m_StateTimer >= m_TransitionDuration * 4.0f)
			{
				m_CurrentSubState = GameplaySubState::AnimLakitu1;
				m_StateTimer = 0.0f;
				m_LakituPos.y = -32.0f;
			}
			break;
		}

		case GameplaySubState::AnimLakitu1:
		{
			m_RaceCurrentTime = m_RaceStartTime;

			m_LakituPos.y += 80.0f * dt; // 下降速度
			if (m_LakituPos.y >= 7.0f)   // 假設下降到 Y = 45 的位置停止
			{
				m_LakituPos.y = 7.0f;
				m_CurrentSubState = GameplaySubState::AnimLakitu2;
				m_StateTimer = 0.0f;
			}
			break;
		}
		case GameplaySubState::AnimLakitu2:
		{
			m_RaceCurrentTime = m_RaceStartTime;

			m_LakituPos.y -= 8.0f * dt;
			if (m_LakituPos.y <= 5.0f)
			{
				m_LakituPos.y = 5.0f;
				m_CurrentSubState = GameplaySubState::AnimLakituLight1;
				m_StateTimer = 0.0f;
			}
			break;
		}
		case GameplaySubState::AnimLakituLight1:
		{
			m_RaceStartSFX->Play();

			m_RaceCurrentTime = m_RaceStartTime;

			m_StateTimer += dt;
			if (m_StateTimer >= 1.0f)
			{
				m_CurrentSubState = GameplaySubState::AnimLakituLight2;
				m_StateTimer = 0.0f;
			}
			break;
		}
		case GameplaySubState::AnimLakituLight2:
		{
			m_StateTimer += dt;
			if (m_StateTimer >= 1.0f)
			{
				m_CurrentSubState = GameplaySubState::AnimLakituLight3;
				m_StateTimer = 0.0f;

				m_RaceStartTime = SDL_GetTicks();
				m_RaceCurrentTime = m_RaceStartTime;

				m_CurrentLapStartTime = m_RaceStartTime;
			}
			break;
		}
		case GameplaySubState::AnimLakituLight3:
		{
			m_MarioCircuitBGM->FadeIn(0, -1);

			UpdateKart(dt);

			m_StateTimer += dt;
			if (m_StateTimer >= 1.0f)
			{
				m_CurrentSubState = GameplaySubState::AnimLakitu3;
				m_StateTimer = 0.0f;
			}
			break;
		}
		case GameplaySubState::AnimLakitu3:
		{
			UpdateKart(dt);

			m_LakituPos.y -= 120.0f * dt;
			if (m_LakituPos.y <= -40.0f)
			{
				m_MarioCircuitBGM->FadeIn(200, -1);

				m_CurrentSubState = GameplaySubState::Game;
			}
			break;
		}

		case GameplaySubState::AnimFinish:
		{
			m_StateTimer += dt;
			m_Drivers[m_DriverIndex].Velocity *= 0.94f;
			m_Drivers[m_DriverIndex].Position += m_Drivers[m_DriverIndex].Velocity * (dt);

			// camera 繞到正面
			float angleDiff = GetShortestAngle(m_TargetCameraYaw, m_CameraYaw);
			if (std::abs(angleDiff) > 0.02f)
			{
				m_CameraYaw += 2.4f * dt;
			}
			else
			{
				m_CameraYaw = m_TargetCameraYaw;
			}
			UpdateCameraAndBackground();
			UpdateKartSpriteRotation();

			// Lakitu 位移邏輯
			if (m_LakituPos.y < 20.0f)
			{
				m_LakituPos.y += 50.0f * dt;
			}
			m_LakituPos.x += 20.0f * dt;

			if (m_StateTimer > 1.6f)
			{
				m_LakituPos.y -= 100.0f * dt;
				if (m_LakituPos.y < -50.0f)
				{
					m_CurrentSubState = GameplaySubState::ShowScore;
					m_MarioCircuitRanksBGM->FadeIn(200, -1);
				}
				break;
			}
		}

		case GameplaySubState::ShowScore:
		{
			if (Util::Input::IsKeyDown(Util::Keycode::SPACE))
			{
				m_MarioCircuitRanksBGM->FadeOut(800);
				m_CurrentSubState = GameplaySubState::AnimToMenu;
				m_StateTimer = 0.0f;
			}
			break;
		}
		case GameplaySubState::AnimToMenu:
		{
			m_StateTimer += dt;
			if (m_StateTimer >= m_TransitionDuration * 4.0f)
			{
				m_CurrentSubState = GameplaySubState::End;
				m_StateTimer = 0.0f;
			}
			break;
		}

		case GameplaySubState::End:
		{
			app->ChangeState(std::make_unique<MenuState>());
			break;
		}

		default:
		{
			UpdateKart(dt);
			if (m_LapCount > m_LevelData.totalLaps)
			{
				m_CurrentSubState = GameplaySubState::AnimFinish;
			}
			break;
		}
		}

		// 處理鍵盤 R 重來
		if (Util::Input::IsKeyDown(Util::Keycode::R))
		{
			ResetScene();
			UpdateKart(0);
			m_CurrentSubState = GameplaySubState::Begin;
		}
	}
	void GameplayState::Render(Game::Application::App* app)
	{
		const auto& layout = Game::GameCore::RETRO_LAYOUT;

		glDisable(GL_SCISSOR_TEST);
		glViewport(0, 0, layout.width, layout.height);

		RenderScene();

		std::shared_ptr<Util::Image> currentLakituImg = nullptr;
		switch (m_CurrentSubState)
		{
		case GameplaySubState::Begin:
		{
			float inverseProgress =
				1.0f - (m_StateTimer / (m_TransitionDuration * 4.0f));
			ApplyFade(inverseProgress);
			return;
		}

		case GameplaySubState::AnimLakitu1:
		case GameplaySubState::AnimLakitu2:
		{
			currentLakituImg = m_LakituLight[0]; // 準備就緒（熄燈）
			break;
		}
		case GameplaySubState::AnimLakituLight1:
		{
			currentLakituImg = m_LakituLight[1]; // 紅燈 1
			break;
		}
		case GameplaySubState::AnimLakituLight2:
		{
			currentLakituImg = m_LakituLight[2]; // 紅燈 2
			break;
		}
		case GameplaySubState::AnimLakituLight3:
		case GameplaySubState::AnimLakitu3:
		{
			currentLakituImg = m_LakituLight[3]; // 綠燈
			break;
		}

		case GameplaySubState::AnimFinish:
		{
			int pingPongFrames[] = { 0, 1, 2, 1 };
			int currentFrame = (int)(m_StateTimer * 8.0f) % 4;
			currentLakituImg = m_LakituFlag[pingPongFrames[currentFrame]];
			break;
		}

		case GameplaySubState::ShowScore:
		{
			bool isRenderTotalTime = (SDL_GetTicks() / 32) % 2 == 0;

			m_UIRenderer->DrawImage(m_UiLapTimeScoreboardIcon, 80.0f, 21.0f);

			uint32_t totalTime = 0;
			for (int lap = 0; lap < m_LapTimes.size(); lap++)
			{
				totalTime += m_LapTimes[lap];
				uint32_t mins = (m_LapTimes[lap] / 60000) % 100;
				uint32_t secs = (m_LapTimes[lap] / 1000) % 60;
				uint32_t ms = (m_LapTimes[lap] / 10) % 100;
				m_UIRenderer->DrawImage(m_UiScoreNumberIcon[mins % 10], 112.0f, 37.0f + 9.0f * lap);
				m_UIRenderer->DrawImage(m_UiScoreNumberIcon[(secs / 10) % 10], 128.0f, 37.0f + 9.0f * lap);
				m_UIRenderer->DrawImage(m_UiScoreNumberIcon[secs % 10], 136.0f, 37.0f + 9.0f * lap);
				m_UIRenderer->DrawImage(m_UiScoreNumberIcon[(ms / 10) % 10], 152.0f, 37.0f + 9.0f * lap);
				m_UIRenderer->DrawImage(m_UiScoreNumberIcon[ms % 10], 160.0f, 37.0f + 9.0f * lap);
			}
			if (isRenderTotalTime)
			{
				uint32_t totalMins = (totalTime / 60000) % 100;
				uint32_t totalSecs = (totalTime / 1000) % 60;
				uint32_t totalMs = (totalTime / 10) % 100;
				m_UIRenderer->DrawImage(m_UiScoreRedNumberIcon[totalMins % 10], 112.0f, 89.0f);
				m_UIRenderer->DrawImage(m_UiScoreRedNumberIcon[(totalSecs / 10) % 10], 128.0f, 89.0f);
				m_UIRenderer->DrawImage(m_UiScoreRedNumberIcon[totalSecs % 10], 136.0f, 89.0f);
				m_UIRenderer->DrawImage(m_UiScoreRedNumberIcon[(totalMs / 10) % 10], 152.0f, 89.0f);
				m_UIRenderer->DrawImage(m_UiScoreRedNumberIcon[totalMs % 10], 160.0f, 89.0f);
			}
			else
			{
				uint32_t totalMins = (totalTime / 60000) % 100;
				uint32_t totalSecs = (totalTime / 1000) % 60;
				uint32_t totalMs = (totalTime / 10) % 100;
				m_UIRenderer->DrawImage(m_UiScoreNumberIcon[totalMins % 10], 112.0f, 89.0f);
				m_UIRenderer->DrawImage(m_UiScoreNumberIcon[(totalSecs / 10) % 10], 128.0f, 89.0f);
				m_UIRenderer->DrawImage(m_UiScoreNumberIcon[totalSecs % 10], 136.0f, 89.0f);
				m_UIRenderer->DrawImage(m_UiScoreNumberIcon[(totalMs / 10) % 10], 152.0f, 89.0f);
				m_UIRenderer->DrawImage(m_UiScoreNumberIcon[totalMs % 10], 160.0f, 89.0f);
			}

			m_UIRenderer->Render();
			break;
		}
		case GameplaySubState::AnimToMenu:
		{
			float progress = m_StateTimer / (m_TransitionDuration * 4.0f);
			ApplyFade(progress);
			break;
		}

		default:
		{
			currentLakituImg = nullptr;
			break;
		}
		}

		if (currentLakituImg && m_LakituPos.y > -40.0f)
		{
			float drawX = m_LakituPos.x;
			if (m_CurrentSubState == GameplaySubState::AnimFinish)
			{
				drawX -= (currentLakituImg->GetSize().x * 0.5f);
			}

			m_UIRenderer->DrawImage(currentLakituImg, drawX, m_LakituPos.y);
			m_UIRenderer->Render();
		}

		// Lakitu lap 渲染
		if (m_LapAnimActive)
		{
			auto img = m_LakituLap[m_CurrentAnimLap - 2];
			if (img)
			{
				m_UIRenderer->DrawImage(img, m_LapAnimPos.x, m_LapAnimPos.y);
				m_UIRenderer->Render();
			}
		}
	}
	void GameplayState::End(Game::Application::App* app)
	{}
}
