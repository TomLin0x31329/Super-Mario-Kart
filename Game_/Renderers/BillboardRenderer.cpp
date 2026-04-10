#include "Renderers/BillboardRenderer.h"

#include "GameCore/ObjectDefinition.h"

#include <cmath>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


namespace
{
	// 計算最短旋轉角度 (-PI 到 PI)
	float GetAngleDifference(float target, float current)
	{
		float diff = std::fmod(target - current, 2.0f * M_PI);
		if (diff > M_PI) diff -= 2.0f * M_PI;
		if (diff < -M_PI) diff += 2.0f * M_PI;
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
		// 1. 建立 1x1 單位方形 (原點在左上角)
		float vertices[] = {
			0.0f, 1.0f,   0.0f, 1.0f, // 左下
			1.0f, 0.0f,   1.0f, 0.0f, // 右上
			0.0f, 0.0f,   0.0f, 0.0f, // 左上

			0.0f, 1.0f,   0.0f, 1.0f, // 左下
			1.0f, 1.0f,   1.0f, 1.0f, // 右下
			1.0f, 0.0f,   1.0f, 0.0f  // 右上
		};

		glGenVertexArrays(1, &quadVAO_);
		glGenBuffers(1, &quadVBO_);
		glBindVertexArray(quadVAO_);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		// Position attribute (0)
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		// UV attribute (1)
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
		glEnableVertexAttribArray(1);

		// 2. 載入 UI Shader (正交投影用)
		program_ = std::make_unique<Core::Program>(
			RESOURCE_DIR "/shaders/ui.vert",
			RESOURCE_DIR "/shaders/ui.frag"
		);
	}
	void BillboardRenderer::Draw(
		glm::vec2 cameraPos, float cameraYaw, float cameraPitch,
		const std::vector<BillboardItem>& items)
	{
		std::vector<RenderCommand> drawQueue;

		// ==========================================
		// 1. 批次計算所有物件的 3D 深度與 2D 螢幕座標
		// ==========================================
		for (const auto& item : items)
		{
			if (!item.definition || item.definition->lods.empty()) continue;

			// 計算相對向量
			glm::vec2 rel = item.worldPos - cameraPos;
			glm::vec2 forward = { std::sin(cameraYaw), -std::cos(cameraYaw) };
			glm::vec2 right = { std::cos(cameraYaw), std::sin(cameraYaw) };

			float depth = glm::dot(rel, forward);
			float side = glm::dot(rel, right);

			// 剔除：在攝影機背後
			if (depth <= 0.001f || depth > item.definition->lods.back().maxDistance) continue;

			float horizon = 1.0f + cameraPitch;
			float cameraHeight = 120.0f;
			float fov = 0.42f;

			// 計算原始的 screenX, screenY (0.0 ~ 1.0)
			float screenX = (side / depth) / (2.0f * fov) + 0.5f;
			float screenY = horizon - (cameraHeight / depth);

			// 剔除：超出畫面太多
			if (screenX < -0.5f || screenX > 1.5f || screenY < -0.5f || screenY > 1.5f) continue;

			// 轉換為 FBO 像素座標 (Bottom-Up 系統的 Y)
			const auto& layout = Game::GameCore::RETRO_LAYOUT;
			float fboX = screenX * 256.0f;
			float fboY_fromBottom = screenY * layout.track + layout.GetTrackY();


			// ==========================================
			// 2. 自動依照透視選擇圖片 (核心魔法)
			// ==========================================
			const std::vector<Game::GameCore::DirectionalSprite>* bestSprites = nullptr;
			float minHeightDiff = 99999.0f;

			// 遍歷所有載入的圖片，找出「真實高度」跟「理想透視高度」最接近的那張！
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

			// ==========================================
			// 3. 1:1 像素完美繪製 (無縮放)
			// ==========================================
			float imgW = targetImage->GetSize().x;
			float imgH = targetImage->GetSize().y;

			float drawX = fboX - (imgW * 0.5f);
			float drawY = 224.0f - (fboY_fromBottom + imgH);

			GLuint texID = targetImage->GetTextureId();
			drawQueue.push_back({ drawX, drawY, imgW, imgH, depth, texID });
		}

		// ==========================================
		// 4. 畫家演算法 (Painter's Algorithm) - 由遠畫到近
		// ==========================================
		std::sort(drawQueue.begin(), drawQueue.end(), [](const RenderCommand& a, const RenderCommand& b) {
			return a.depth > b.depth;
			});

		// ==========================================
		// 5. 執行純 OpenGL 渲染管線
		// ==========================================
		if (drawQueue.empty()) return;

		program_->Bind();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);

		// 設定正交投影矩陣 (左上角為 0,0)
		glm::mat4 projection = glm::ortho(0.0f, 256.0f, 224.0f, 0.0f, -1.0f, 1.0f);
		GLint projLoc = glGetUniformLocation(program_->GetId(), "u_Projection");
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

		glBindVertexArray(quadVAO_);
		GLint modelLoc = glGetUniformLocation(program_->GetId(), "u_Model");
		GLint texLoc = glGetUniformLocation(program_->GetId(), "u_Texture");
		glUniform1i(texLoc, 0);

		// 依序繪製所有立體物件
		for (const auto& cmd : drawQueue)
		{
			// 設定 Model 矩陣 (位移 + 縮放)
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(cmd.x, cmd.y, 0.0f));
			model = glm::scale(model, glm::vec3(cmd.width, cmd.height, 1.0f));
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

			glActiveTexture(GL_TEXTURE0);

			// 🌟 注意：取得 OpenGL Texture ID 的方式
			// 假設你的 Util::Image 底層使用的是 Core::Texture，請透過適當的方法取得 GLuint
			// 如果你的 Util::Image 不支援 GetTexture()->GetId()，請在引擎源碼中加一個 Getter
			GLuint textureId = cmd.textureId;
			glBindTexture(GL_TEXTURE_2D, textureId);

			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
	}
}
