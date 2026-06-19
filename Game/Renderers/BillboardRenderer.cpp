#include "Renderers/BillboardRenderer.h"
#include "GameCore/ObjectDefinition.h"

#include <cmath>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace
{
	float GetAngleDifference(float target, float current)
	{
		float diff = std::fmod(target - current, 2.0f * (float)M_PI);
		if (diff > (float)M_PI) diff -= 2.0f * (float)M_PI;
		if (diff < -(float)M_PI) diff += 2.0f * (float)M_PI;
		return diff;
	}
}

namespace Game::Renderer
{
	BillboardRenderer::~BillboardRenderer()
	{
		if (quadVAO_ != 0) {
			glDeleteVertexArrays(1, &quadVAO_);
			glDeleteBuffers(1, &quadVBO_);
		}
	}

	void BillboardRenderer::Init()
	{
		float vertices[] = {
			0.0f, 1.0f,   0.0f, 1.0f,
			1.0f, 0.0f,   1.0f, 0.0f,
			0.0f, 0.0f,   0.0f, 0.0f,

			0.0f, 1.0f,   0.0f, 1.0f,
			1.0f, 1.0f,   1.0f, 1.0f,
			1.0f, 0.0f,   1.0f, 0.0f
		};
		glGenVertexArrays(1, &quadVAO_);
		glGenBuffers(1, &quadVBO_);
		glBindVertexArray(quadVAO_);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
		glEnableVertexAttribArray(1);

		program_ = std::make_unique<Core::Program>(
			RESOURCE_DIR "/shaders/ui.vert",
			RESOURCE_DIR "/shaders/ui.frag"
		);
	}

	void BillboardRenderer::Draw(
		glm::vec2 cameraPos,
		float cameraYaw,
		float cameraPitch,
		float cameraFOV,
		float cameraHeight,
		float regionY, float regionHeight,
		const std::vector<BillboardItem>& items)
	{
		std::vector<RenderCommand> drawQueue;

		for (const auto& item : items)
		{
			if (!item.definition || item.definition->lods.empty()) continue;

			glm::vec2 rel = item.worldPos - cameraPos;
			glm::vec2 forward = { std::sin(cameraYaw), -std::cos(cameraYaw) };
			glm::vec2 right = { std::cos(cameraYaw), std::sin(cameraYaw) };

			float depth = glm::dot(rel, forward);
			float side = glm::dot(rel, right);

			if (depth <= 0.001f || depth > item.definition->lods.back().maxDistance) continue;

			float horizon = 1.0f + cameraPitch;
			float screenX = (side / depth) / (2.0f * cameraFOV) + 0.5f;
			float screenY = horizon - (cameraHeight / depth);

			if (screenX < -0.5f || screenX > 1.5f || screenY < -0.5f || screenY > 1.5f) continue;

			const auto& layout = Game::GameCore::RETRO_LAYOUT;
			float fboX = screenX * 256.0f;
			float fboY_fromBottom = screenY * regionHeight + regionY;

			const std::vector<Game::GameCore::DirectionalSprite>* bestSprites = nullptr;
			for (const auto& lod : item.definition->lods)
			{
				if (depth <= lod.maxDistance)
				{
					bestSprites = &lod.sprites;
					break;
				}
			}
			if (!bestSprites) continue;

			std::shared_ptr<Util::Image> targetImage = nullptr;
			glm::vec2 dirToCamera = cameraPos - item.worldPos;
			float angleToCamera = std::atan2(-dirToCamera.y, dirToCamera.x);
			float relativeViewAngle = GetAngleDifference(angleToCamera, item.yaw);

			float minAngleDiff = 9999.0f;
			for (const auto& sprite : *bestSprites) {
				float diff = std::abs(GetAngleDifference(relativeViewAngle, sprite.angle));
				if (diff < minAngleDiff) {
					minAngleDiff = diff;
					targetImage = sprite.image;
				}
			}
			if (!targetImage) continue;

			float imgW = targetImage->GetSize().x;
			float imgH = targetImage->GetSize().y;
			float drawX = fboX - (imgW * 0.5f);
			float drawY = 224.0f - (fboY_fromBottom + imgH);

			GLuint texID = targetImage->GetTextureId();

			// 🌟 就是這裡修復了 Initializer List 報錯
			drawQueue.push_back(RenderCommand{ drawX, drawY, imgW, imgH, depth, texID, item.shouldFlip });
		}

		std::sort(drawQueue.begin(), drawQueue.end(), [](const RenderCommand& a, const RenderCommand& b) {
			return a.depth > b.depth;
			});

		if (drawQueue.empty()) return;

		program_->Bind();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);

		GLboolean cullFaceEnabled = glIsEnabled(GL_CULL_FACE);
		glDisable(GL_CULL_FACE);

		glm::mat4 projection = glm::ortho(0.0f, 256.0f, 224.0f, 0.0f, -1.0f, 1.0f);
		GLint projLoc = glGetUniformLocation(program_->GetId(), "u_Projection");
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

		glBindVertexArray(quadVAO_);
		GLint modelLoc = glGetUniformLocation(program_->GetId(), "u_Model");
		GLint texLoc = glGetUniformLocation(program_->GetId(), "u_Texture");
		glUniform1i(texLoc, 0);

		for (const auto& cmd : drawQueue)
		{
			glm::mat4 model = glm::mat4(1.0f);
			if (cmd.shouldFlip)
			{
				model = glm::translate(model, glm::vec3(cmd.x + cmd.width, cmd.y, 0.0f));
				model = glm::scale(model, glm::vec3(-cmd.width, cmd.height, 1.0f));
			}
			else
			{
				model = glm::translate(model, glm::vec3(cmd.x, cmd.y, 0.0f));
				model = glm::scale(model, glm::vec3(cmd.width, cmd.height, 1.0f));
			}

			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
			glActiveTexture(GL_TEXTURE0);

			// 🌟 就是這裡修復了 glBindTexture 參數不足的報錯
			glBindTexture(GL_TEXTURE_2D, cmd.textureId);

			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		if (cullFaceEnabled) {
			glEnable(GL_CULL_FACE);
		}
	}
}
