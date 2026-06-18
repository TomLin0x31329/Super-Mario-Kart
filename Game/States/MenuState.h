#pragma once

#include "pch.hpp"
#include "Util/Image.hpp"
#include "Util/BGM.hpp"
#include "Util/SFX.hpp"

#include "GameState.h"
#include "Renderers/ScrollingUIRenderer.h"
#include "Renderers/UIRenderer.h"

namespace Game::States
{
	enum class MenuSubState
	{
		AnimFromGame,
		Begin,
		AnimToPlayerCount,      // transition
		SelectPlayerCount,
		AnimToRule1,            // transition
		AnimToRule2,            // transition
		SelectRule,
		AnimToCheck1,           // transition
		AnimToCheck2,           // transition
		Check,
		AnimToCharacterSelect1, // transition
		AnimToCharacterSelect2, // transition
		AnimCancelCheck,        // transition to AnimToPlayerCount

		CharacterSelect,
		AnimToGame,				// transition
		End
	};
	struct DriverUIItem
	{
		std::vector<std::shared_ptr<Util::Image>> Sprites;
		int IdleSpriteIndex = 7;

		int CurrentFrame = IdleSpriteIndex;
		bool IdleYOffset = false;
		bool IsSelected = false;
		bool IsCheck = false;
		float AnimTimer = 0.0f;

		void Update(float dt, bool isSelected, bool isCheck)
		{
			IsSelected = isSelected;
			IsCheck = isCheck;

			AnimTimer += dt;
			if (!IsCheck)
			{
				if (AnimTimer > 0.1f)
				{
					CurrentFrame = !isSelected ?
						IdleSpriteIndex :
						(CurrentFrame + 1) % (Sprites.size() * 2 - 2);
					AnimTimer = 0.0f;
					IdleYOffset = !IdleYOffset;
				}
			}
			else
			{
				if (AnimTimer > 0.06f)
				{
					if (CurrentFrame < 11) CurrentFrame++;
					else if (CurrentFrame > 11) CurrentFrame--;
					AnimTimer = 0.0f;
				}
			}
		}
		std::shared_ptr<Util::Image> GetCurrentImage()
		{
			return Sprites[
				CurrentFrame >= Sprites.size() ? 
					Sprites.size() - 2 - (CurrentFrame - Sprites.size()) :
					CurrentFrame];
		}
		bool IsNeedFlip()
		{
			return CurrentFrame >= Sprites.size() ? true : false;
		}
	};
	class MenuState : public GameState
	{
	private:
		std::unique_ptr<Renderer::UIRenderer> m_UIRenderer;
		std::unique_ptr<Renderer::ScrollingUIRenderer> m_ScrollingUIRenderer;

		std::vector<DriverUIItem> m_Drivers;

		// Title 捲動背景參數
		float m_TitleBgOffset = 0.0f;
		float m_TitleAutoScrollSpeed = 0.2f;
		// CharacterSelect 捲動背景參數
		float m_CharacterSelectDriverBgOffset = 0.0f;
		float m_CharacterSelectDriverBgAutoScrollSpeed = 1.6f;
		float m_CharacterSelectChooseYourDriverOffset = 0.0f;
		float m_CharacterSelectChooseYourDriverAutoScrollSpeed = 0.16f;

		// Title 選項參數
		float m_TransitionTimer = 0.0f;           // 當前經過的時間
		const float m_TransitionDuration = 0.2f;  // 動畫總長度
		bool m_IsTransitioning = false;           // 是否正在播放展開動畫

		// Menu 選擇結果
		int m_PlayerCount = 0;
		int m_RuleType = 0;
		int m_DriverIndex = 0;
		bool m_isSelectOK = true;
		bool m_isSelectDriver = false;
		bool m_isCheckDriver = false;

		// Menu 狀態機
		MenuSubState m_CurrentSubState = MenuSubState::AnimFromGame;

		std::shared_ptr<Util::Image> m_BlackImage;

		std::shared_ptr<Util::Image> m_TitleSelectIconImage;
		std::shared_ptr<Util::Image> m_TitleImage;
		std::shared_ptr<Util::Image> m_TitleBgImage;
		std::shared_ptr<Util::Image> m_TitlePlayerCountSelectImage;
		std::shared_ptr<Util::Image> m_TitleRuleSelectImage;
		std::shared_ptr<Util::Image> m_TitleCheckYesImage;
		std::shared_ptr<Util::Image> m_TitleCheckNoImage;

		std::shared_ptr<Util::Image> m_CharacterSelectBgImage;
		std::shared_ptr<Util::Image> m_CharacterSelectDriverBgImage;
		std::shared_ptr<Util::Image> m_CharacterSelectChooseYourDriverImage;
		std::shared_ptr<Util::Image> m_CharacterSelect1PImage;
		std::shared_ptr<Util::Image> m_CharacterSelect1POKImage;
		std::shared_ptr<Util::Image> m_CharacterSelect1POKYellowImage;

		std::unique_ptr<Util::BGM> m_MenuBGM;
		std::unique_ptr<Util::BGM> m_CharacterSelectBGM;

		std::unique_ptr<Util::SFX> m_MenuSelectSFX;
		std::unique_ptr<Util::SFX> m_MenuMoveSFX;

	private:
		void RenderTitleAndBgPage();
		void RenderPlayerCountSelectPage();
		void RenderRuleSelectPage();
		void RenderCheckPage();
		void RenderCharacterSelectPage();

		void ApplyCenterScissor(float progress, glm::vec2 size, glm::vec2 center);
		void ApplyFade(float alpha);

	public:
		MenuState();

	public:
		void Start(Game::Application::App* app) override;
		void Update(Game::Application::App* app, float dt) override;
		void Render(Game::Application::App* app) override;
		void End(Game::Application::App* app) override;

	};
}
