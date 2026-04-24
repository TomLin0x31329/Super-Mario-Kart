#pragma once

#include "pch.hpp"

#include "GameCore/ObjectDefinition.h"

namespace Game::Gameplay
{
	class GameObject;

	struct PlayerData
	{
		glm::vec2 position;

		std::function<void(GameObject* thisPtr, int deltaCoins)> onCollectionCoins;
		std::function<void(GameObject* thisPtr, int checkpointId)> onPathCheckpoint;
		std::function<void(GameObject* thisPtr, float speedPenalty, float spinEffect)> onCollisionEffect;
		std::function<void(GameObject* thisPtr)> onErrorDirection;
	};
	using CollisionCallback = std::function<void(GameObject* self, GameObject* other)>;
	enum class ObjectType { Player, Coin, Pipe, ItemBox };
	class GameObject
	{
	private:
		Game::GameCore::ObjectDefinition definition_;
		glm::vec2 position_{ 0 };
		bool isActive_ = true;

		ObjectType type_;              
		CollisionCallback onCollide_;  

	public:
		GameObject(
			glm::vec2 position,
			Game::GameCore::ObjectDefinition definition,
			ObjectType type,
			CollisionCallback onCollide = nullptr)
			: position_(position),
			definition_(std::move(definition)),
			type_(type),
			onCollide_(std::move(onCollide))
		{
		}

	public:
		void SetPosition(glm::vec2 newPos) { position_ = newPos; }

		const Game::GameCore::ObjectDefinition& GetDefinition() const { return definition_; }
		glm::vec2 GetPosition() const { return position_; }
		bool IsActive() const { return isActive_; }
		ObjectType GetType() const { return type_; }

		void Deactivate() { isActive_ = false; }
		void Activate() { isActive_ = true; }

		void UpdateCollision(GameObject& other)
		{
			if (!isActive_ || !other.IsActive()) return;
			if (this == &other) return;

			float dist = glm::length(position_ - other.GetPosition());
			if (dist < definition_.collisionRadius + other.GetDefinition().collisionRadius)
			{
				if (this->onCollide_) this->onCollide_(this, &other);
				if (other.onCollide_) other.onCollide_(&other, this);
			}
		}

	};
}
