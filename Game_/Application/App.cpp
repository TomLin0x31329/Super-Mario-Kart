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
			m_PipeDefinition.collisionRadius = 16.0f;
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

			float debugSpeed = 320.0f;     // 調整這個數值來改變前進快慢
			float debugTurnSpeed = 1.6f;   // 調整這個數值來改變旋轉快慢
			float dt = 0.016f;

			// 1. 幽浮攝影機轉向
			if (Util::Input::IsKeyPressed(Util::Keycode::A))
				m_CameraYaw -= debugTurnSpeed * dt;
			if (Util::Input::IsKeyPressed(Util::Keycode::D))
				m_CameraYaw += debugTurnSpeed * dt;

			// 2. 幽浮攝影機前後移動 (注意 m_CameraPos 現在是 vec2)
			glm::vec2 forwardDir = { sin(m_CameraYaw), -cos(m_CameraYaw) };
			if (Util::Input::IsKeyPressed(Util::Keycode::W))
				m_CameraPos += forwardDir * debugSpeed * dt;
			if (Util::Input::IsKeyPressed(Util::Keycode::S))
				m_CameraPos -= forwardDir * debugSpeed * dt;

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
			m_UIRenderer->DrawImage(uiTimeNumberIcon_[0], 176.0f, 7.0f);
			m_UIRenderer->DrawImage(uiTimeNumberIcon_[0], 184.0f, 7.0f);
			m_UIRenderer->DrawImage(uiTimeNumberIcon_[10], 192.0f, 7.0f);
			m_UIRenderer->DrawImage(uiTimeNumberIcon_[0], 200.0f, 7.0f);
			m_UIRenderer->DrawImage(uiTimeNumberIcon_[0], 208.0f, 7.0f);
			m_UIRenderer->DrawImage(uiTimeNumberIcon_[10], 216.0f, 7.0f);
			m_UIRenderer->DrawImage(uiTimeNumberIcon_[10], 219.0f, 7.0f);
			m_UIRenderer->DrawImage(uiTimeNumberIcon_[0], 224.0f, 7.0f);
			m_UIRenderer->DrawImage(uiTimeNumberIcon_[0], 232.0f, 7.0f);

			// Kart
			m_UIRenderer->DrawImage(uiScoreKartIcon_, 186.0f, 85.0f);
			m_UIRenderer->DrawImage(uiScoreNumberIcon_[10], 198.0f, 85.0f);
			m_UIRenderer->DrawImage(uiScoreNumberIcon_[3], 206.0f, 85.0f);
			// Coin
			m_UIRenderer->DrawImage(uiScoreCoinIcon_, 190.0f, 92.0f);
			m_UIRenderer->DrawImage(uiScoreNumberIcon_[10], 198.0f, 93.0f);
			m_UIRenderer->DrawImage(uiScoreNumberIcon_[0], 206.0f, 93.0f);
			m_UIRenderer->DrawImage(uiScoreNumberIcon_[0], 214.0f, 93.0f);

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
