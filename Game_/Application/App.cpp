#include "App.h"

#include <iostream>
#include "Util/Image.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"

#include <SDL_image.h>
#include "Core/Texture.hpp"
#include "Core/TextureUtils.hpp"
#include "GameCore/RetroLayout.h"

#include "Gameplay/PlayerKart.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace
{
	// 計算兩個角度之間的最短距離 (-PI 到 PI)
	float GetShortestAngle(float target, float current)
	{
		float diff = std::fmod(target - current, 2.0f * (float)M_PI);
		if (diff > (float)M_PI) diff -= 2.0f * (float)M_PI;
		if (diff < -(float)M_PI) diff += 2.0f * (float)M_PI;
		return diff;
	}
}

namespace Game::Application
{
	void App::Start()
	{
		LOG_TRACE("Start");

		// --- 創建 FBO ---
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

		// --- 初始化 Renderers ---
		{
			// 初始化天空與樹木背景
			m_SkyboxRenderer = std::make_unique<Game::Renderer::BackgroundRenderer>(
				RESOURCE_DIR "/sprites/tracks/level_1/sky.png", 2.0f);
			m_SkyboxRenderer->Init();

			m_TreesRenderer = std::make_unique<Game::Renderer::BackgroundRenderer>(
				RESOURCE_DIR "/sprites/tracks/level_1/trees.png", 3.5f);
			m_TreesRenderer->Init();

			// 初始化 Mode 7 賽道
			m_BakedRenderer = std::make_unique<Game::Renderer::BakedSurfaceRenderer>();
			m_BakedRenderer->Init(
				RESOURCE_DIR "/sprites/tracks/level_1/track.png",
				RESOURCE_DIR "/sprites/tracks/level_1/out_of_bounds.png"
			);

			// 初始化告示牌渲染器
			m_BillboardRenderer = std::make_unique<Game::Renderer::BillboardRenderer>();
			m_BillboardRenderer->Init();

			// 初始化 UI 渲染器
			m_UIRenderer = std::make_unique<Game::Renderer::UIRenderer>();
			m_UIRenderer->Init();
		}

		// --- 初始化場景物件 ---
		{
			// 讀取關卡資料
			levelData_ = Game::Systems::LevelLoader::LoadLevel(RESOURCE_DIR "/Levels/Level1.json");

			// 設置pipe LOD
			m_PipeDefinition.collisionRadius = 6.0f;
			float lodDistances[12] = {
				120.0f,   // 1.png 
				160.0f,  // 2.png
				200.0f,  // 3.png
				240.0f,  // 4.png
				280.0f,  // 5.png
				320.0f,  // 6.png
				380.0f,  // 7.png
				440.0f,  // 8.png
				600.0f,  // 9.png
				800.0f, // 10.png
				2000.0f, // 11.png
				2400.0f  // 12.png
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

			// ==========================================
			// 🌟 1. 初始化所有地圖上的 GameObject (物理碰撞體)
			// ==========================================
			m_LevelObjects.clear(); // 確保清空

			// 初始化金幣
			Game::GameCore::ObjectDefinition coinDef;
			coinDef.collisionRadius = 6.0f; // 金幣比較小，碰撞半徑設小一點
			for (const auto& pos : levelData_.coins)
			{
				m_LevelObjects.emplace_back(
					pos * 1.6f, // 🌟 極度重要：因為你的視覺座標放大了 1.6 倍，物理座標也要對應放大！
					coinDef,
					Game::Gameplay::ObjectType::Coin
				);
			}
			// 初始化道具箱
			Game::GameCore::ObjectDefinition itemBoxDef;
			itemBoxDef.collisionRadius = 14.0f;
			for (const auto& box : levelData_.itemBoxes)
			{
				m_LevelObjects.emplace_back(
					box.pos * 1.6f,
					itemBoxDef,
					Game::Gameplay::ObjectType::ItemBox
				);
			}
			// 初始化水管
			for (const auto& pos : levelData_.pipes)
			{
				m_LevelObjects.emplace_back(
					pos * 1.6f,
					m_PipeDefinition, // 水管直接沿用上面設定好的 LOD 定義
					Game::Gameplay::ObjectType::Pipe
				);
			}
			LOG_INFO("Successfully initialized {} physical level objects.", m_LevelObjects.size());


			// 載入金幣圖片並蓋印到賽道上
			SDL_Surface* coinSurface = IMG_Load(RESOURCE_DIR "/sprites/items/coin.png");
			if (coinSurface)
			{
				m_BakedRenderer->BakeObject(coinSurface, levelData_.coins);
				SDL_FreeSurface(coinSurface);
				LOG_INFO("Successfully baked {} coins onto the track.", levelData_.coins.size());
			}
			else
			{
				LOG_ERROR("Failed to load coin sprite for baking: {}", IMG_GetError());
			}

			// 載入 Item box 圖片並透過蓋印到賽道上
			SDL_Surface* itemBoxSurface = IMG_Load(RESOURCE_DIR "/sprites/items/item_panels.png");
			if (itemBoxSurface)
			{
				// 🌟 建立一個通用的烘焙佇列
				std::vector<Game::Renderer::RotatedBakeItem> bakeItems;

				// 🌟 由 App 來負責「翻譯」：把遊戲邏輯的 ItemBoxData 轉成純渲染指令
				for (const auto& box : levelData_.itemBoxes) {
					bakeItems.push_back({ box.pos, box.direction });
				}

				// 把翻譯好的指令丟給渲染器
				m_BakedRenderer->BakeRotatedObjects(itemBoxSurface, bakeItems);

				SDL_FreeSurface(itemBoxSurface);
				LOG_INFO("Successfully baked {} item boxes onto the track.", levelData_.itemBoxes.size());
			}

			// 初始化 PlayerKart
			m_Player.Init();
			Game::GameCore::ObjectDefinition playerDef;
			playerDef.collisionRadius = 4.0f;
			m_PlayerCollider = std::make_unique<Game::Gameplay::GameObject>(
				m_Player.Position,
				playerDef,
				Game::Gameplay::ObjectType::Player,
				[this](Game::Gameplay::GameObject* self, Game::Gameplay::GameObject* other) {
					// 🌟 當玩家碰到東西時會觸發這裡！
					if (other->GetType() == Game::Gameplay::ObjectType::Coin) {
						LOG_INFO("Hit coin");
						m_coinCount++;
						other->Deactivate(); // 讓金幣消失
						m_BakedRenderer->RestoreBakedObject({ 7, 7 }, { other->GetPosition() / 1.6f });
					}
					else if (other->GetType() == Game::Gameplay::ObjectType::Pipe) {
						LOG_INFO("Hit pipe");
						// 可以在這裡讓 m_Player 減速或是彈開
						m_Player.Velocity *= 0.3f;
					}
				}
			);
		}

		// --- 初始化 UI 物件 ---
		{
			// Timer
			uiItemSlotIcon_ = std::make_shared<Util::Image>(
				RESOURCE_DIR "/sprites/ui/item_slot.png");
			for (int i = 0; i < 10; ++i)
			{
				uiTimeNumberIcon_[i] = std::make_shared<Util::Image>(
					RESOURCE_DIR "/sprites/ui/time/number/" + std::to_string(i) + ".png");
			}
			uiTimeNumberIcon_[10] = std::make_shared<Util::Image>(
				RESOURCE_DIR "/sprites/ui/time/number/comma.png");

			// Score
			uiScoreKartIcon_ = std::make_shared<Util::Image>(
				RESOURCE_DIR "/sprites/ui/score/kart.png");
			uiScoreCoinIcon_ = std::make_shared<Util::Image>(
				RESOURCE_DIR "/sprites/ui/score/coin.png");
			for (int i = 0; i < 10; ++i)
			{
				uiScoreNumberIcon_[i] = std::make_shared<Util::Image>(
					RESOURCE_DIR "/sprites/ui/score/number/" + std::to_string(i) + ".png");
			}
			uiScoreNumberIcon_[10] = std::make_shared<Util::Image>(
				RESOURCE_DIR "/sprites/ui/score/number/multiply.png");

		}

		// --- 初始化攝像機 ---
		{
			if (!levelData_.startingGrid.empty())
			{
				m_CameraPos.x = levelData_.startingGrid[0].pos.x * 1.6;
				m_CameraPos.y = levelData_.startingGrid[0].pos.y * 1.6;
				m_CameraYaw = levelData_.initialYaw;
			}
		}

		m_RaceStartTime = SDL_GetTicks();
		m_coinCount = 0;

		m_CurrentState = State::UPDATE;
	}
	void App::Update()
	{
		// --- 處理輸入和更新 ---
		{
			// 關閉程式
			if (Util::Input::IfExit() || Util::Input::IsKeyUp(Util::Keycode::ESCAPE))
			{
				m_CurrentState = State::END;
				return;
			}


			// 1. 更新卡丁車物理
			float dt = 0.016f; // (實務上應該用真正的 DeltaTime)
			m_Player.Update(dt, levelData_.collisionMap);
			m_PlayerCollider->SetPosition(m_Player.Position);

			for (auto& obj : m_LevelObjects)
			{
				if (obj.IsActive())
				{
					// 讓玩家的代理碰撞體，去跟場景上的每一個物件做碰撞測試
					m_PlayerCollider->UpdateCollision(obj);
				}
			}

			// ==========================================
			// 2. 鏡頭綁定 (Camera Follow)
			// ==========================================
			// 鏡頭位置跟隨車子位置
			m_CameraPos = m_Player.Position;

			// 🌟 鏡頭方向跟隨「實際飛行方向 (Velocity)」，而不是車頭方向！
			// 如果車子有速度才更新鏡頭角度，否則維持原樣
			if (glm::length(m_Player.Velocity) > 5.0f)
			{
				// 🌟 鏡頭的朝向，直接從「實際前進向量 (Velocity)」提取角度！
				float velocityAngle = std::atan2(m_Player.Velocity.x, -m_Player.Velocity.y);

				// 使用最短路徑平滑追蹤鏡頭 (確保畫面不會抖動)
				float angleDiff = GetShortestAngle(velocityAngle, m_CameraYaw);
				m_CameraYaw += angleDiff * 0.15f;

				// 防止浮點數溢位
				m_CameraYaw = std::fmod(m_CameraYaw, 2.0f * (float)M_PI);
				if (m_CameraYaw > (float)M_PI) m_CameraYaw -= 2.0f * (float)M_PI;
				if (m_CameraYaw < -(float)M_PI) m_CameraYaw += 2.0f * (float)M_PI;
			}
			// 取得攝影機目前看過去的方向
			glm::vec2 cameraForward = { std::sin(m_CameraYaw), -std::cos(m_CameraYaw) };

			// 攝影機距離車子的 3D 深度距離
			// 這個數字決定了旋轉軸心，150.0f 是一個完美的基準值，你可以微調它
			float cameraDistance = 150.0f;

			// 🌟 把攝影機從車子的位置「往後退」，讓車子成為旋轉的圓心！
			m_CameraPos = m_Player.Position - cameraForward * cameraDistance;

			if (Util::Input::IsKeyDown(Util::Keycode::R))
			{
				m_Player.Init();
				m_Player.Position = levelData_.startingGrid[0].pos * 1.6f;
				m_Player.ExpectedDir = { 0.0f, -1.0f };
				m_Player.Velocity = { 0.0f, -0.01f };

				m_CameraYaw = 0.0f;

				m_RaceStartTime = SDL_GetTicks();
				m_coinCount = 0;
			}
			if (Util::Input::IsKeyDown(Util::Keycode::SPACE))
			{
				LOG_INFO("Camera Pos -> X: {:.1f}, Z: {:.1f}",
					m_CameraPos.x / 1.6, m_CameraPos.y / 1.6);
			}
		}

		// --- 清除上一幀畫面 ---
		{
			glBindFramebuffer(GL_FRAMEBUFFER, m_Fbo);
			glViewport(0, 0,
				Game::GameCore::RETRO_LAYOUT.width,
				Game::GameCore::RETRO_LAYOUT.height);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}

		// --- 渲染場景 ---
		{
			// 開啟剪裁測試
			glEnable(GL_SCISSOR_TEST);
			const auto& layout = Game::GameCore::RETRO_LAYOUT;
			glScissor(0, layout.GetTrackY(), layout.width, layout.height - layout.GetTrackY());

			// 繪製背景
			glViewport(0, Game::GameCore::RETRO_LAYOUT.GetSkyY(),
				Game::GameCore::RETRO_LAYOUT.width,
				Game::GameCore::RETRO_LAYOUT.sky);
			m_SkyboxRenderer->Draw(m_CameraYaw);
			m_TreesRenderer->Draw(m_CameraYaw);
			// 繪製賽道
			glViewport(0, Game::GameCore::RETRO_LAYOUT.GetTrackY(),
				Game::GameCore::RETRO_LAYOUT.width,
				Game::GameCore::RETRO_LAYOUT.track);
			m_BakedRenderer->Draw(m_CameraPos, m_CameraYaw, 0.04f);

			// 恢復 Viewport
			glViewport(0, 0,
				Game::GameCore::RETRO_LAYOUT.width,
				Game::GameCore::RETRO_LAYOUT.height);

			// 繪製水管
			std::vector<Game::Renderer::BillboardItem> renderItems;
			for (const auto& pipePos : levelData_.pipes)
			{
				Game::Renderer::BillboardItem item;
				item.worldPos.x = pipePos.x * 1.6f;
				item.worldPos.y = pipePos.y * 1.6f;
				item.yaw = 0.0f;
				item.definition = &m_PipeDefinition;

				renderItems.push_back(item);
			}
			m_BillboardRenderer->Draw(m_CameraPos, m_CameraYaw, 0.04f, renderItems);

			// 關閉剪裁測試
			glDisable(GL_SCISSOR_TEST);
		}
		// --- 渲染 UI ---
		{
			// Item Slot
			m_UIRenderer->DrawImage(uiItemSlotIcon_, 147.0f, 5.0f);
			// Timer
			uint32_t currentTimeMs = SDL_GetTicks() - m_RaceStartTime;
			uint32_t minutes = (currentTimeMs / 60000) % 100;
			uint32_t seconds = (currentTimeMs / 1000) % 60;
			uint32_t centiseconds = (currentTimeMs / 10) % 100;
			int m10 = minutes / 10;
			int m1 = minutes % 10;
			int s10 = seconds / 10;
			int s1 = seconds % 10;
			int ms10 = centiseconds / 10;
			int ms1 = centiseconds % 10;
			m_UIRenderer->DrawImage(uiTimeNumberIcon_[m10], 176.0f, 7.0f);
			m_UIRenderer->DrawImage(uiTimeNumberIcon_[m1], 184.0f, 7.0f);
			m_UIRenderer->DrawImage(uiTimeNumberIcon_[10], 192.0f, 7.0f);
			m_UIRenderer->DrawImage(uiTimeNumberIcon_[s10], 200.0f, 7.0f);
			m_UIRenderer->DrawImage(uiTimeNumberIcon_[s1], 208.0f, 7.0f);
			m_UIRenderer->DrawImage(uiTimeNumberIcon_[10], 216.0f, 7.0f);
			m_UIRenderer->DrawImage(uiTimeNumberIcon_[10], 219.0f, 7.0f);
			m_UIRenderer->DrawImage(uiTimeNumberIcon_[ms10], 224.0f, 7.0f);
			m_UIRenderer->DrawImage(uiTimeNumberIcon_[ms1], 232.0f, 7.0f);

			// Kart
			m_UIRenderer->DrawImage(uiScoreKartIcon_, 186.0f, 85.0f);
			m_UIRenderer->DrawImage(uiScoreNumberIcon_[10], 198.0f, 85.0f);
			m_UIRenderer->DrawImage(uiScoreNumberIcon_[3], 206.0f, 85.0f);
			// Coin
			m_UIRenderer->DrawImage(uiScoreCoinIcon_, 190.0f, 92.0f);
			m_UIRenderer->DrawImage(uiScoreNumberIcon_[10], 198.0f, 93.0f);
			m_UIRenderer->DrawImage(uiScoreNumberIcon_[m_coinCount / 10], 206.0f, 93.0f);
			m_UIRenderer->DrawImage(uiScoreNumberIcon_[m_coinCount % 10], 214.0f, 93.0f);

			m_UIRenderer->Render();
		}

		// --- 渲染 PlayerKart ---
		{
			auto kartImage = m_Player.CurrentSprite;
			if (kartImage)
			{
				float kartDrawX = 128.0f - (kartImage->GetSize().x * 0.5f);
				float kartDrawY = 80.0f - (kartImage->GetSize().y * 0.5f);

				m_UIRenderer->DrawImage(kartImage, kartDrawX, kartDrawY, m_Player.ShouldFlipSprite);
			}
			m_UIRenderer->Render();
		}

		m_Root.Update();

		// --- 放大 FBO 並顯示 ---
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
	void App::End()
	{
		LOG_TRACE("End");
	}
}
