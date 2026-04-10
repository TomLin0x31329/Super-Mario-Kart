#pragma once

#include "pch.hpp"

#include "Core/Program.hpp"
#include "Core/VertexArray.hpp"
#include "Core/Texture.hpp"

#include "GameCore/RetroLayout.h"
#include "Gameplay/GameObject.h"

namespace Game::Renderer
{
	class BackgroundRenderer
	{
	private:
		std::unique_ptr<Core::Program> program_;
		std::unique_ptr<Core::VertexArray> vertexArray_;
		std::unique_ptr<Core::Texture> texture_;

		std::string imagePath_;
		float scrollSpeedMultiplier_ = 1.0f;

		glm::vec2 textureSize_ = { 1.0f, 1.0f };

	public:
		BackgroundRenderer(const std::string& imagePath, float scrollSpeed);
		BackgroundRenderer(const BackgroundRenderer&) = delete;
		BackgroundRenderer& operator=(const BackgroundRenderer&) = delete;
		~BackgroundRenderer() = default;

	public:
		bool Init();
		void Draw(float cameraYaw);

	};
}
