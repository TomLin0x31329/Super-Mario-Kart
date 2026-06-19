#include "PlayerKart.h"

#include "Util/Input.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


namespace Game::Gameplay
{
	glm::vec2 PlayerKart::RotateVector(glm::vec2 v, float angleRad)
	{
		float c = std::cos(angleRad);
		float s = std::sin(angleRad);
		return { v.x * c - v.y * s, v.x * s + v.y * c };
	}
	float PlayerKart::GetAngleDifference(float target, float current)
	{
		float diff = std::fmod(target - current, 2.0f * M_PI);
		if (diff > M_PI) diff -= 2.0f * M_PI;
		if (diff < -M_PI) diff += 2.0f * M_PI;
		return diff;
	}
}
namespace Game::Gameplay
{
	void PlayerKart::Init(std::string name)
	{
		for (int i = 1; i <= 12; ++i)
		{
			m_SpritesLod0.push_back(std::make_shared<Util::Image>(
				RESOURCE_DIR "/sprites/karts/" + name + "/lod_0/" + std::to_string(i) + ".PNG"));
			m_SpritesLod1.push_back(std::make_shared<Util::Image>(
				RESOURCE_DIR "/sprites/karts/" + name + "/lod_1/" + std::to_string(i) + ".PNG"));
			m_SpritesLod2.push_back(std::make_shared<Util::Image>(
				RESOURCE_DIR "/sprites/karts/" + name + "/lod_2/" + std::to_string(i) + ".PNG"));
		}
		m_CurrentSprite = m_SpritesLod0[0];
	}
	void PlayerKart::Update(float dt, const Game::GameCore::CollisionMap& colMap, float cameraYaw)
	{
		float currentSpeed = glm::length(Velocity);
		float currentMaxSpeed = m_MaxSpeed;
		float currentDecel = m_Deceleration;


		// 檢查玩家「當前腳下」是否在草地
		auto CheckTerrain = [&](float testX, float testY, bool& outIsWall, bool& outIsGrass) {
			outIsWall = false;
			outIsGrass = false;
			int mapX = static_cast<int>(testX / 1.6f);
			int mapY = static_cast<int>(testY / 1.6f);
			uint8_t r = 0, g = 0, b = 0;

			if (colMap.GetPixelColor(mapX, mapY, r, g, b))
			{
				if (r == 0 && g == 0 && b == 0) outIsGrass = true;
				else if (r == 104 && g == 104 && b == 248) outIsWall = true;
			}
			else
			{
				outIsWall = true; // 掉出地圖外視同撞牆
			}
			};

		bool isWall = false, isGrass = false;
		CheckTerrain(Position.x, Position.y, isWall, isGrass);
		if (isGrass) {
			currentMaxSpeed = m_MaxSpeed * 0.4f;
			currentDecel = m_Deceleration * 4.0f;
		}

		bool isDrifting = Util::Input::IsKeyPressed(Util::Keycode::SPACE);
		float turnFactor = isDrifting ? m_DriftTurnFactor : m_NormalTurnFactor;

		// 玩家輸入
		{
			if (currentSpeed > 10.0f)
			{
				float turnAngle = 0.0f;
				if (Util::Input::IsKeyPressed(Util::Keycode::A) || Util::Input::IsKeyPressed(Util::Keycode::LEFT)) turnAngle -= m_TurnRate * dt;
				if (Util::Input::IsKeyPressed(Util::Keycode::D) || Util::Input::IsKeyPressed(Util::Keycode::RIGHT)) turnAngle += m_TurnRate * dt;

				if (turnAngle != 0.0f) {
					ExpectedDir = RotateVector(ExpectedDir, turnAngle);
					ExpectedDir = glm::normalize(ExpectedDir);
					Velocity = RotateVector(Velocity, turnAngle * turnFactor);
				}
			}
			if (Util::Input::IsKeyPressed(Util::Keycode::W) || Util::Input::IsKeyPressed(Util::Keycode::UP))
			{
				float currentAccel = 0.0f;
				for (const auto& tier : m_AccelerationCurve) {
					if (currentSpeed <= tier.speedLimit) { currentAccel = tier.accelRate; break; }
				}
				Velocity += ExpectedDir * currentAccel * dt;
			}
			else if (Util::Input::IsKeyPressed(Util::Keycode::S) || Util::Input::IsKeyPressed(Util::Keycode::DOWN))
			{
				if (currentSpeed > 0.1f) Velocity -= glm::normalize(Velocity) * m_BrakePower * dt;
			}
			if (currentSpeed > 0.1f && !(Util::Input::IsKeyPressed(Util::Keycode::W) || Util::Input::IsKeyPressed(Util::Keycode::UP)))
			{
				float dragDrop = m_Deceleration * dt;
				if (dragDrop > currentSpeed) dragDrop = currentSpeed;
				Velocity -= glm::normalize(Velocity) * dragDrop;
			}
		}

		// 抓地力計算
		{
			if (glm::length(Velocity) > currentMaxSpeed)
			{
				Velocity = glm::mix(Velocity, glm::normalize(Velocity) * currentMaxSpeed, 5.0f * dt);
			}
			currentSpeed = glm::length(Velocity);
			if (currentSpeed > 10.0f)
			{
				float currentGrip = isDrifting ? m_Grip * 0.1f : m_Grip;
				if (glm::dot(Velocity, ExpectedDir) < 0.0f)
				{
					currentGrip *= 0.05f;
				}
				glm::vec2 idealVelocity = ExpectedDir * currentSpeed;
				Velocity = glm::mix(Velocity, idealVelocity, currentGrip * dt);
			}
		}

		// 碰撞處理
		{
			m_IsHitWall = false;

			float kartRadius = 4.0f;

			float offsetX = (Velocity.x > 0.0f) ? kartRadius : -kartRadius;
			float nextX = Position.x + Velocity.x * dt;
			CheckTerrain(nextX + offsetX, Position.y, isWall, isGrass);
			if (isWall)
			{
				m_IsHitWall = true;

				float sign = (Velocity.x > 0.0f) ? -1.0f : 1.0f;
				float bounceSpeed = std::abs(Velocity.x) * m_BounceFactor;
				if (bounceSpeed < 60.0f) bounceSpeed = 60.0f;
				Velocity.x = sign * bounceSpeed;
				Position.x += sign * 2.0f;
			}
			else
			{
				Position.x = nextX;
			}

			float offsetY = (Velocity.y > 0.0f) ? kartRadius : -kartRadius;
			float nextY = Position.y + Velocity.y * dt;
			CheckTerrain(Position.x, nextY + offsetY, isWall, isGrass);
			if (isWall)
			{
				m_IsHitWall = true;

				float sign = (Velocity.y > 0.0f) ? -1.0f : 1.0f;
				float bounceSpeed = std::abs(Velocity.y) * m_BounceFactor;
				if (bounceSpeed < 60.0f) bounceSpeed = 60.0f;
				Velocity.y = sign * bounceSpeed;
				Position.y += sign * 2.0f;
			}
			else
			{
				Position.y = nextY;
			}
		}

		GetSpriteForCamera(cameraYaw, m_CurrentSprite, m_ShouldFlipSprite);
	}

	void PlayerKart::GetSpriteForCamera(
		float cameraYaw, std::shared_ptr<Util::Image>& outSprite, bool& outFlip, int lod)
	{
		float expectedAngle = std::atan2(ExpectedDir.x, -ExpectedDir.y);
		float deltaAngle = GetAngleDifference(expectedAngle, cameraYaw);
		float deltaDeg = deltaAngle * 180.0f / M_PI;

		float lookupAngle = 0.0f;
		if (deltaDeg < 0)
		{
			lookupAngle = std::abs(deltaDeg);
			outFlip = true;
		}
		else
		{
			lookupAngle = deltaDeg;
			outFlip = false;
		}

		lookupAngle = std::clamp(lookupAngle, 0.0f, 180.0f);

		auto it = std::lower_bound(m_MarioKartSpritesLUT.begin(), m_MarioKartSpritesLUT.end(), lookupAngle,
			[](const std::pair<float, int>& a, float b) {
				return a.first < b;
			});

		int spriteIndex = 1;
		if (it == m_MarioKartSpritesLUT.end()) 
		{
			spriteIndex = m_MarioKartSpritesLUT.back().second;
		}
		else 
		{
			spriteIndex = it->second;
		}

		if (lod == 0)	outSprite = m_SpritesLod0[spriteIndex - 1];
		else if (lod == 1)	 outSprite = m_SpritesLod1[spriteIndex - 1];
		else if (lod == 2)	 outSprite = m_SpritesLod2[spriteIndex - 1];
	}
}
