#include "pch.hpp"

#include "MenuState.h"
#include "GameplayState.h"

#include "Application/App.h"
#include "Util/Input.hpp"

#include <memory>


namespace Game::States
{
	void MenuState::RenderTitleAndBgPage()
	{
		m_ScrollingUIRenderer->Draw(
			m_TitleBgImage, 0.0f, 0.0f, 256.0f, 224.0f, m_TitleBgOffset, 0.0f);

		m_UIRenderer->DrawImage(m_TitleImage, 11.0f, 25.0f);
		m_UIRenderer->Render();
	}
	void MenuState::RenderPlayerCountSelectPage()
	{
		m_UIRenderer->DrawImage(m_TitlePlayerCountSelectImage, 64.0f, 120.0f);
		m_UIRenderer->DrawImage(m_TitleSelectIconImage, 80.0f, 127.0f + m_PlayerCount * 16.0f);
		m_UIRenderer->Render();
	}
	void MenuState::RenderRuleSelectPage()
	{
		m_UIRenderer->DrawImage(m_TitleRuleSelectImage, 64.0f, 120.0f);
		m_UIRenderer->DrawImage(m_TitleSelectIconImage, 72.0f, 127.0f + m_RuleType * 16.0f);
		m_UIRenderer->Render();
	}
	void MenuState::RenderCheckPage()
	{
		m_UIRenderer->DrawImage(m_isSelectOK ? m_TitleCheckYesImage : m_TitleCheckNoImage, 64.0f, 112.0f);
		m_UIRenderer->Render();
	}
	void MenuState::RenderCharacterSelectPage()
	{
		// Render Bg & Driver
		{
			// 渲染 Driver 動態背景
			for (int y = 0; y < 2; y++)
			{
				for (int x = 0; x < 4; x++)
				{
					m_ScrollingUIRenderer->Draw(
						m_CharacterSelectDriverBgImage,
						36.0f + 48.0f * x, 71.0f + 64.0f * y,
						48.0f, 56.0f, m_CharacterSelectDriverBgOffset, 0.0f);
				}
			}
			// 渲染跑馬燈
			m_ScrollingUIRenderer->Draw(
				m_CharacterSelectChooseYourDriverImage,
				80.0f, 23.0f, 256.0f, 16.0f, m_CharacterSelectChooseYourDriverOffset, 0.0f);
			// 渲染背景
			m_UIRenderer->DrawImage(m_CharacterSelectBgImage, 0.0f, 0.0f);

			// 渲染 Driver
			for (int i = 0; i < 8; i++)
			{
				if (m_Drivers[i].Sprites.size() == 0) continue;

				if (!m_Drivers[i].IsSelected)
				{
					m_UIRenderer->DrawImage(
						m_Drivers[i].GetCurrentImage(),
						40.0f + 48.0f * (i % 4),
						80.0f + 63.0f * (i / 4) + (m_Drivers[i].IdleYOffset),
						m_Drivers[i].IsNeedFlip());
				}
				else if (m_Drivers[i].IsSelected)
				{
					m_UIRenderer->DrawImage(
						m_Drivers[i].GetCurrentImage(),
						40.0f + 48.0f * (i % 4),
						80.0f + 63.0f * (i / 4),
						m_Drivers[i].IsNeedFlip());
				}
			}
		}

		// Draw 1P & Check button
		{
			m_UIRenderer->DrawImage(
				m_CharacterSelect1PImage,
				48.0f + 48.0f * (m_DriverIndex % 4),
				63.0f + 63.0f * (m_DriverIndex / 4));
			if (m_isSelectDriver && !m_isCheckDriver)
			{
				m_UIRenderer->DrawImage(
					m_CharacterSelect1POKImage, 151.0f, 190.0f);
			}
			else if (m_isCheckDriver)
			{
				m_UIRenderer->DrawImage(
					m_CharacterSelect1POKYellowImage, 151.0f, 190.0f);
			}
		}

		m_UIRenderer->Render();
	}

	void MenuState::ApplyCenterScissor(float progress, glm::vec2 size, glm::vec2 center)
	{
		const auto& layout = Game::GameCore::RETRO_LAYOUT;
		float w = size.x * progress;
		float h = size.y * progress;
		float glCenterY = layout.height - center.y;

		glEnable(GL_SCISSOR_TEST);
		glScissor((GLint)(center.x - w * 0.5f), (GLint)(glCenterY - h * 0.5f), (GLsizei)w, (GLsizei)h);
	}
	void MenuState::ApplyFade(float alpha)
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
	MenuState::MenuState()
	{
	}
}
namespace Game::States
{
	void MenuState::Start(Game::Application::App* app)
	{
		m_UIRenderer = std::make_unique<Game::Renderer::UIRenderer>();
		m_UIRenderer->Init();
		m_ScrollingUIRenderer = std::make_unique<Game::Renderer::ScrollingUIRenderer>();
		m_ScrollingUIRenderer->Init();

		m_BlackImage = std::make_shared<Util::Image>(
			RESOURCE_DIR "/sprites/ui/menu/black.png");

		m_TitleImage = std::make_shared<Util::Image>(
			RESOURCE_DIR "/sprites/ui/menu/title.png");
		m_TitleBgImage = std::make_shared<Util::Image>(
			RESOURCE_DIR "/sprites/ui/menu/title_background.png");
		m_TitlePlayerCountSelectImage = std::make_shared<Util::Image>(
			RESOURCE_DIR "/sprites/ui/menu/title_playerCountSelect.png");
		m_TitleRuleSelectImage = std::make_shared<Util::Image>(
			RESOURCE_DIR "/sprites/ui/menu/title_ruleSelect.png");
		m_TitleCheckYesImage = std::make_shared<Util::Image>(
			RESOURCE_DIR "/sprites/ui/menu/title_check_Yes.png");
		m_TitleCheckNoImage = std::make_shared<Util::Image>(
			RESOURCE_DIR "/sprites/ui/menu/title_check_No.png");

		m_TitleSelectIconImage = std::make_shared<Util::Image>(
			RESOURCE_DIR "/sprites/ui/menu/title_selectIcon.png");
		m_CharacterSelectBgImage = std::make_shared<Util::Image>(
			RESOURCE_DIR "/sprites/ui/menu/characterselect_background.png");
		m_CharacterSelectDriverBgImage = std::make_shared<Util::Image>(
			RESOURCE_DIR "/sprites/ui/menu/characterSelect_DriverBackground.png");
		m_CharacterSelectChooseYourDriverImage = std::make_shared<Util::Image>(
			RESOURCE_DIR "/sprites/ui/menu/characterSelect_ChooseYourDriver.png");
		m_CharacterSelect1PImage = std::make_shared<Util::Image>(
			RESOURCE_DIR "/sprites/ui/menu/characterSelect_1P.png");
		m_CharacterSelect1POKImage = std::make_shared<Util::Image>(
			RESOURCE_DIR "/sprites/ui/menu/characterSelect_1POK.png");
		m_CharacterSelect1POKYellowImage = std::make_shared<Util::Image>(
			RESOURCE_DIR "/sprites/ui/menu/characterSelect_1POK_yellow.png");

		m_Drivers.resize(8);
		for (int i = 0; i < 12; i++)
		{
			m_Drivers[0].Sprites.push_back(std::make_shared<Util::Image>(
				RESOURCE_DIR "/sprites/karts/mario_kart/lod_0/" + std::to_string(i + 1) + ".png"));
		}
		m_Drivers[1].IdleYOffset = false;
		for (int i = 0; i < 12; i++)
		{
			m_Drivers[1].Sprites.push_back(std::make_shared<Util::Image>(
				RESOURCE_DIR "/sprites/karts/peach_kart/lod_0/" + std::to_string(i + 1) + ".png"));
		}
		m_Drivers[1].IdleYOffset = true;


		m_MenuBGM = std::make_unique<Util::BGM>(RESOURCE_DIR "/audio/bgm/1-02. Title Screen.mp3");
		m_MenuBGM->SetVolume(64);
		m_CharacterSelectBGM = std::make_unique<Util::BGM>(RESOURCE_DIR "/audio/bgm/1-03. Main Menu.mp3");
		m_CharacterSelectBGM->SetVolume(64);

		m_MenuSelectSFX = std::make_unique<Util::SFX>(RESOURCE_DIR "/audio/sfx/menuselect.wav");
		m_MenuSelectSFX->SetVolume(32);
		m_MenuMoveSFX = std::make_unique<Util::SFX>(RESOURCE_DIR "/audio/sfx/menumove.wav");
		m_MenuMoveSFX->SetVolume(32);

		m_TransitionTimer = 0;
	}
	void MenuState::Update(Game::Application::App* app, float dt)
	{
		// 切換狀態機
		switch (m_CurrentSubState)
		{
		case MenuSubState::AnimFromGame:
		{
			m_MenuBGM->FadeIn(0, -1);

			m_TransitionTimer += dt;
			if (m_TransitionTimer >= m_TransitionDuration * 4.0f)
			{
				m_CurrentSubState = MenuSubState::Begin;
				m_TransitionTimer = 0.0f;
			}
			break;
		}

			// Begin
		case MenuSubState::Begin:
		{
			m_TitleBgOffset += m_TitleAutoScrollSpeed * dt;
			if (Util::Input::IsKeyDown(Util::Keycode::SPACE))
			{
				m_MenuSelectSFX->Play();

				m_CurrentSubState = MenuSubState::AnimToPlayerCount;
				m_TransitionTimer = 0.0f;
			}
			break;
		}
		case MenuSubState::AnimToPlayerCount:
		{
			m_TransitionTimer += dt;
			if (m_TransitionTimer >= m_TransitionDuration)
			{
				m_CurrentSubState = MenuSubState::SelectPlayerCount;
				m_TransitionTimer = 0.0f;
			}
			break;
		}

		// Select PlayerCount
		case MenuSubState::SelectPlayerCount:
		{
			if (Util::Input::IsKeyDown(Util::Keycode::SPACE) && m_PlayerCount == 0)
			{
				m_MenuSelectSFX->Play();

				m_CurrentSubState = MenuSubState::AnimToRule1;
				m_TransitionTimer = 0.0f;
			}
			else if (Util::Input::IsKeyDown(Util::Keycode::W) ||
				Util::Input::IsKeyDown(Util::Keycode::S))
			{
				m_MenuMoveSFX->Play();

				m_PlayerCount = m_PlayerCount == 1 ? 0 : 1;
			}
			break;
		}
		case MenuSubState::AnimToRule1:
		{
			m_TransitionTimer += dt;
			if (m_TransitionTimer >= m_TransitionDuration)
			{
				m_CurrentSubState = MenuSubState::AnimToRule2;
				m_TransitionTimer = 0.0f;

			}
			break;
		}
		case MenuSubState::AnimToRule2:
		{
			m_TransitionTimer += dt;
			if (m_TransitionTimer >= m_TransitionDuration)
			{
				m_CurrentSubState = MenuSubState::SelectRule;
				m_TransitionTimer = 0.0f;
			}
			break;
		}

		// Select Rule
		case MenuSubState::SelectRule:
		{
			if (Util::Input::IsKeyDown(Util::Keycode::SPACE) && m_RuleType == 1)
			{
				m_MenuSelectSFX->Play();

				m_CurrentSubState = MenuSubState::AnimToCheck1;
				m_TransitionTimer = 0.0f;
			}
			else if (Util::Input::IsKeyDown(Util::Keycode::W) ||
				Util::Input::IsKeyDown(Util::Keycode::S))
			{
				m_MenuMoveSFX->Play();

				m_RuleType = m_RuleType == 1 ? 0 : 1;
			}
			break;
		}
		case MenuSubState::AnimToCheck1:
		{
			m_TransitionTimer += dt;
			if (m_TransitionTimer >= m_TransitionDuration)
			{
				m_CurrentSubState = MenuSubState::AnimToCheck2;
				m_TransitionTimer = 0.0f;
			}
			break;
		}
		case MenuSubState::AnimToCheck2:
		{
			m_TransitionTimer += dt;
			if (m_TransitionTimer >= m_TransitionDuration)
			{
				m_CurrentSubState = MenuSubState::Check;
				m_TransitionTimer = 0.0f;
			}
			break;
		}

		// Check
		case MenuSubState::Check:
		{
			if (Util::Input::IsKeyDown(Util::Keycode::SPACE))
			{
				m_MenuSelectSFX->Play();

				m_CurrentSubState = m_isSelectOK ?
					MenuSubState::AnimToCharacterSelect1 : MenuSubState::AnimCancelCheck;
				m_TransitionTimer = 0.0f;
				m_isSelectOK = true;
			}
			else if (Util::Input::IsKeyDown(Util::Keycode::A) ||
				Util::Input::IsKeyDown(Util::Keycode::D))
			{
				m_MenuMoveSFX->Play();

				m_isSelectOK = !m_isSelectOK;
			}
			break;
		}
		case MenuSubState::AnimToCharacterSelect1:
		{
			m_MenuBGM->FadeOut(1000);

			m_TransitionTimer += dt;
			if (m_TransitionTimer >= m_TransitionDuration * 4.0f)
			{
				m_CurrentSubState = MenuSubState::AnimToCharacterSelect2;
				m_TransitionTimer = 0.0f;
			}
			break;
		}
		case MenuSubState::AnimToCharacterSelect2:
		{
			m_CharacterSelectBGM->FadeIn(200, -1);

			m_TransitionTimer += dt;
			if (m_TransitionTimer >= m_TransitionDuration * 4.0f)
			{
				m_CurrentSubState = MenuSubState::CharacterSelect;
				m_TransitionTimer = 0.0f;
			}
			break;
		}
		case MenuSubState::AnimCancelCheck:
		{
			m_TransitionTimer += dt;
			if (m_TransitionTimer >= m_TransitionDuration)
			{
				m_CurrentSubState = MenuSubState::AnimToPlayerCount;
				m_TransitionTimer = 0.0f;
			}
			break;
		}

		// Character Select
		case MenuSubState::CharacterSelect:
		{
			m_CharacterSelectDriverBgOffset += m_CharacterSelectDriverBgAutoScrollSpeed * dt;
			m_CharacterSelectChooseYourDriverOffset += m_CharacterSelectChooseYourDriverAutoScrollSpeed * dt;

			if (!m_isSelectDriver)
			{
				if (Util::Input::IsKeyDown(Util::Keycode::W) ||
					Util::Input::IsKeyDown(Util::Keycode::S))
				{
					m_MenuMoveSFX->Play();

					if (Util::Input::IsKeyDown(Util::Keycode::W))
					{
						m_DriverIndex = m_DriverIndex % 4;
					}
					if (Util::Input::IsKeyDown(Util::Keycode::S))
					{
						m_DriverIndex = (m_DriverIndex % 4) + 4;
					}
				}
				if (Util::Input::IsKeyDown(Util::Keycode::A) ||
					Util::Input::IsKeyDown(Util::Keycode::D))
				{
					m_MenuMoveSFX->Play();

					if (Util::Input::IsKeyDown(Util::Keycode::A))
					{
						m_DriverIndex--;
						if (m_DriverIndex == -1) m_DriverIndex = 7;
					}
					if (Util::Input::IsKeyDown(Util::Keycode::D))
					{
						m_DriverIndex++;
						if (m_DriverIndex == 8) m_DriverIndex = 0;
					}
				}

				if (Util::Input::IsKeyDown(Util::Keycode::SPACE) &&
					m_Drivers[m_DriverIndex].Sprites.size() != 0)
				{
					m_MenuSelectSFX->Play();

					m_isSelectDriver = true;
				}
			}
			else if (!m_isCheckDriver)
			{
				if (Util::Input::IsKeyDown(Util::Keycode::SPACE))
				{
					m_MenuSelectSFX->Play();

					m_CurrentSubState = MenuSubState::AnimToGame;
					m_TransitionTimer = 0.0f;
					m_isCheckDriver = true;
				}
			}

			for (int i = 0; i < 8; i++)
			{
				m_Drivers[i].Update(dt,
					m_DriverIndex == i && m_isSelectDriver,
					m_DriverIndex == i && m_isCheckDriver);
			}
			break;
		}
		case MenuSubState::AnimToGame:
		{
			m_CharacterSelectBGM->FadeOut(1000);

			m_TransitionTimer += dt;
			if (m_TransitionTimer >= m_TransitionDuration * 4.0f)
			{
				m_CurrentSubState = MenuSubState::End;
				m_TransitionTimer = 0.0f;
			}
			for (int i = 0; i < 8; i++)
			{
				m_Drivers[i].Update(dt,
					m_DriverIndex == i && m_isSelectDriver,
					m_DriverIndex == i && m_isCheckDriver);
			}
			break;
		}

		// End
		case MenuSubState::End:
		{
			app->ChangeState(std::make_unique<GameplayState>(m_DriverIndex));
			break;
		}
		}
	}
	void MenuState::Render(Game::Application::App* app)
	{
		const auto& layout = Game::GameCore::RETRO_LAYOUT;

		glDisable(GL_SCISSOR_TEST);
		glViewport(0, 0, layout.width, layout.height);

		// Menu 渲染
		switch (m_CurrentSubState)
		{
		case MenuSubState::AnimFromGame:
		{
			RenderTitleAndBgPage();
			float inverseProgress = 1.0f - (m_TransitionTimer / (m_TransitionDuration * 4.0f));
			ApplyFade(inverseProgress);
			break;
		}

			// Begin
		case MenuSubState::Begin:
		{
			RenderTitleAndBgPage();
			break;
		}
		case MenuSubState::AnimToPlayerCount:
		{
			RenderTitleAndBgPage();
			float progress = m_TransitionTimer / m_TransitionDuration;
			ApplyCenterScissor(progress, { 125.0f, 40.0f }, { 126.5f, 140.0f });
			RenderPlayerCountSelectPage();
			glDisable(GL_SCISSOR_TEST);
		}
		break;

			// Select PlayerCount
		case MenuSubState::SelectPlayerCount:
		{
			RenderTitleAndBgPage();
			RenderPlayerCountSelectPage();
			break;
		}
		case MenuSubState::AnimToRule1:
		{
			RenderTitleAndBgPage();
			float inverseProgress = 1.0f - (m_TransitionTimer / m_TransitionDuration);
			ApplyCenterScissor(inverseProgress, { 125.0f, 40.0f }, { 126.5f, 140.0f });
			RenderPlayerCountSelectPage();
			glDisable(GL_SCISSOR_TEST);
			break;
		}
		case MenuSubState::AnimToRule2:
		{
			RenderTitleAndBgPage();
			float progress = m_TransitionTimer / m_TransitionDuration;
			ApplyCenterScissor(progress, { 125.0f, 40.0f }, { 126.5f, 140.0f });
			RenderRuleSelectPage();
			glDisable(GL_SCISSOR_TEST);
			break;
		}

			// Select Rule
		case MenuSubState::SelectRule:
		{
			RenderTitleAndBgPage();
			RenderRuleSelectPage();
			break;
		}
		case MenuSubState::AnimToCheck1:
		{
			RenderTitleAndBgPage();
			float inverseProgress = 1.0f - (m_TransitionTimer / m_TransitionDuration);
			ApplyCenterScissor(inverseProgress, { 125.0f, 40.0f }, { 126.5f, 140.0f });
			RenderRuleSelectPage();
			glDisable(GL_SCISSOR_TEST);
			break;
		}
		case MenuSubState::AnimToCheck2:
		{
			RenderTitleAndBgPage();
			float progress = m_TransitionTimer / m_TransitionDuration;
			ApplyCenterScissor(progress, { 125.0f, 72.0f }, { 126.5f, 148.0f });
			RenderCheckPage();
			glDisable(GL_SCISSOR_TEST);
			break;
		}

			// Check
		case MenuSubState::Check:
		{
			RenderTitleAndBgPage();
			RenderCheckPage();
			break;
		}
		case MenuSubState::AnimToCharacterSelect1:
		{
			RenderTitleAndBgPage();
			RenderCheckPage();
			float progress = m_TransitionTimer / (m_TransitionDuration * 4.0f);
			ApplyFade(progress);
			break;
		}
		case MenuSubState::AnimToCharacterSelect2:
		{
			RenderCharacterSelectPage();
			float inverseProgress = 1.0f - (m_TransitionTimer / (m_TransitionDuration * 4.0f));
			ApplyFade(inverseProgress);
			break;
		}
		case MenuSubState::AnimCancelCheck:
		{
			RenderTitleAndBgPage();
			float inverseProgress = 1.0f - (m_TransitionTimer / m_TransitionDuration);
			ApplyCenterScissor(inverseProgress, { 125.0f, 40.0f }, { 126.5f, 140.0f });
			RenderCheckPage();
			glDisable(GL_SCISSOR_TEST);
			break;
		}

			// CharacterSelect
		case MenuSubState::CharacterSelect:
		{
			RenderCharacterSelectPage();
			break;
		}
		case MenuSubState::AnimToGame:
		{
			RenderCharacterSelectPage();
			float progress = m_TransitionTimer / (m_TransitionDuration * 4.0f);
			ApplyFade(progress);
			break;
		}
		}
	}
	void MenuState::End(Game::Application::App* app)
	{
	}
}
