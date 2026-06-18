#pragma once

#include "pch.hpp"
#include "Util/Image.hpp"

#include "GameCore/LevelData.h"

namespace Game::Gameplay
{
	struct AccelTier
	{
		float speedLimit;
		float accelRate;
	};

	class PlayerKart
	{
	public:
		glm::vec2 Position = { 920.0f * 1.6f, 589.0f * 1.6f };
		glm::vec2 ExpectedDir = { 0.0f, -1.0f }; // 車頭朝向向量
		glm::vec2 Velocity = { 0.0f, -0.01f };   // 真實移動向量 (包含速度大小)

		// LOD 區間加速度
		std::vector<AccelTier> m_AccelerationCurve = {
			{ 150.0f, 240.0f },  // 起步：加速度適中
			{ 450.0f, 420.0f },  // 中段：爆發力強
			{ 9999.f, 150.0f }   // 尾速：加速度銳減
		};

		float m_MaxSpeed = 400.0f;
		float m_Deceleration = 200.0f; // 自然滑行減阻
		float m_BrakePower = 600.0f;   // 煞車力度

		// 轉向與側滑參數
		float m_TurnRate = 2.0f; // 方向盤轉速 (弧度/秒)
		float m_NormalTurnFactor = 0.4f;
		float m_DriftTurnFactor = 1.8f;
		float m_Grip = 12.0f;
		float m_BounceFactor = 0.6f;

		std::vector<std::shared_ptr<Util::Image>> m_SpritesLod0;
		std::vector<std::shared_ptr<Util::Image>> m_SpritesLod1;
		std::vector<std::shared_ptr<Util::Image>> m_SpritesLod2;
		std::shared_ptr<Util::Image> m_CurrentSprite = nullptr;
		bool m_ShouldFlipSprite = false;

		bool m_IsHitWall = false;

	private:
		const std::vector<std::pair<float, int>> m_MarioKartSpritesLUT = {
			{   0.0f,  1 },
			{  15.0f,  2 },
			{  30.0f,  3 },
			{  45.0f,  4 },
			{  60.0f,  5 },
			{  75.0f,  6 },
			{  90.0f,  7 },
			{ 105.0f,  8 },
			{ 120.0f,  9 },
			{ 135.0f, 10 },
			{ 150.0f, 11 },
			{ 180.0f, 12 }
		};

	private:
		glm::vec2 RotateVector(glm::vec2 v, float angleRad);
		float GetAngleDifference(float target, float current);

	public:
		void Init(std::string name);
		void Update(float dt, const Game::GameCore::CollisionMap& colMap, float cameraYaw);

		void GetSpriteForCamera(
			float cameraYaw, std::shared_ptr<Util::Image>& outSprite, bool& outFlip, int lod = 0);
		bool IsHitWall() const { return m_IsHitWall; }

	};
}
