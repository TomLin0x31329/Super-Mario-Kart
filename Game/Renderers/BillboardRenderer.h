#pragma once

#include "pch.hpp"
#include <vector>
#include <memory>
#include <GL/glew.h>
#include <glm/glm.hpp>

#include "Core/Program.hpp"
#include "GameCore/ObjectDefinition.h"
#include "GameCore/RetroLayout.h"
#include "Util/Image.hpp"

namespace Game::Renderer
{
	struct BillboardItem
	{
		glm::vec2 worldPos;
		float yaw;
		const Game::GameCore::ObjectDefinition* definition;
		bool shouldFlip = false;
	};

	class BillboardRenderer
	{
	private:
		GLuint quadVAO_ = 0;
		GLuint quadVBO_ = 0;
		std::unique_ptr<Core::Program> program_;

		struct RenderCommand 
		{
			float x, y, width, height, depth;
			GLuint textureId;
			bool shouldFlip;
		};

	public:
		BillboardRenderer() = default;
		BillboardRenderer(const BillboardRenderer&) = delete;
		BillboardRenderer& operator=(const BillboardRenderer&) = delete;
		~BillboardRenderer();

	public:
		void Init();
		void Draw(
			glm::vec2 cameraPos,
			float cameraYaw,
			float cameraPitch,
			float cameraFOV, 
			float cameraHeight,
			float regionY, float regionHeight,
			const std::vector<BillboardItem>& items);

	};
}
