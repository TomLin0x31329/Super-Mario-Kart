#pragma once

#include "pch.hpp"

#include "Core/Program.hpp"
#include "Core/VertexArray.hpp"
#include "Core/Texture.hpp"

#include "GameCore/RetroLayout.h"
#include "Gameplay/GameObject.h"

namespace Game::Renderer
{
	class ScrollingUIRenderer
	{
	private:
		std::unique_ptr<Core::Program> program_;
		std::unique_ptr<Core::VertexArray> vertexArray_;

	public:
		ScrollingUIRenderer() = default;
		ScrollingUIRenderer(const ScrollingUIRenderer&) = delete;
		ScrollingUIRenderer& operator=(const ScrollingUIRenderer&) = delete;
		~ScrollingUIRenderer() = default;

	public:
		void Init();
		void Draw(std::shared_ptr<Util::Image> image,
			float x, float y, float width, float height,
			float uvOffsetX, float uvOffsetY,
			bool flipX = false, bool flipY = false, float alpha = 1.0f);

	};
}
