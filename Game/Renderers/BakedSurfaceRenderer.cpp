#include "BakedSurfaceRenderer.h"

#include "Core/TextureUtils.hpp"
#include "Util/Logger.hpp"

namespace Game::Renderer
{
	void BakedSurfaceRenderer::UpdateGLTexture()
	{
		trackTexture_.reset(new Core::Texture(
			Core::SdlFormatToGlFormat(workingTrackSurface_->format->format),
			workingTrackSurface_->w, workingTrackSurface_->h, workingTrackSurface_->pixels));
	}

	BakedSurfaceRenderer::~BakedSurfaceRenderer()
	{
		if (cleanTrackSurface_) SDL_FreeSurface(cleanTrackSurface_);
		if (workingTrackSurface_) SDL_FreeSurface(workingTrackSurface_);
	}

	bool BakedSurfaceRenderer::Init(
		const std::string& trackTexturePath,
		const std::string& outOfBoundsTexturePath)
	{
		// Init Program
		{
			program_ = std::make_unique<Core::Program>(
				RESOURCE_DIR "/shaders/Mode7.vert",
				RESOURCE_DIR "/shaders/Mode7.frag");
			program_->Bind();
			GLint texLoc = glGetUniformLocation(program_->GetId(), "u_Surface");
			glUniform1i(texLoc, 0);
			GLint grassLoc = glGetUniformLocation(program_->GetId(), "u_OutOfBoundsTile");
			glUniform1i(grassLoc, 1);
		}
		// Init VertexArray
		{
			vertexArray_ = std::make_unique<Core::VertexArray>();

			// 2D 畫布 Vertex (-1.0 ~ 1.0)
			vertexArray_->AddVertexBuffer(std::make_unique<Core::VertexBuffer>(
				std::vector<float>{
				-1.0f, +1.0f,
					-1.0f, -1.0f,
					+1.0f, -1.0f,
					+1.0f, +1.0f }, 2));

			// 2D 畫布 UV (0.0 ~ 1.0)
			vertexArray_->AddVertexBuffer(std::make_unique<Core::VertexBuffer>(
				std::vector<float>{
				0.0f, 1.0f,
					0.0f, 0.0f,
					1.0f, 0.0f,
					1.0f, 1.0f }, 2));

			vertexArray_->SetIndexBuffer(std::make_unique<Core::IndexBuffer>(
				std::vector<unsigned int>{ 0, 1, 2, 0, 2, 3 }));
		}

		// Read Track Texture
		{
			cleanTrackSurface_ = IMG_Load(trackTexturePath.c_str());
			trackSize_ = { cleanTrackSurface_->w, cleanTrackSurface_->h };
			if (!cleanTrackSurface_)
			{
				LOG_ERROR("Failed to load Mode 7 texture: {}", IMG_GetError());
				return false;
			}
			workingTrackSurface_ = SDL_ConvertSurface(
				cleanTrackSurface_, cleanTrackSurface_->format, 0);
		}
		// Read OutOfBounds Texture
		{
			SDL_Surface* outOfBoundsSurface = IMG_Load(outOfBoundsTexturePath.c_str());
			if (!outOfBoundsSurface) 
			{
				LOG_ERROR("Failed to load grass tile: {}", IMG_GetError());
				return false;
			}
			outOfBoundsTexture_ = std::make_unique<Core::Texture>(
				Core::SdlFormatToGlFormat(outOfBoundsSurface->format->format),
				outOfBoundsSurface->w, outOfBoundsSurface->h, outOfBoundsSurface->pixels);
			SDL_FreeSurface(outOfBoundsSurface);
		}

		isTrackTextureDirty_ = true;

		return true;
	}
	void BakedSurfaceRenderer::Draw(
		glm::vec2 cameraPos, float cameraYaw, float cameraPitch, float cameraFOV, float cameraHeight)
	{
		if (isTrackTextureDirty_)
		{
			UpdateGLTexture();
			isTrackTextureDirty_ = false;
		}

		program_->Bind();

		GLint posLoc = glGetUniformLocation(program_->GetId(), "u_CameraPos");
		glUniform2f(posLoc, cameraPos.x, cameraPos.y);
		GLint heightLoc = glGetUniformLocation(program_->GetId(), "u_CameraHeight");
		glUniform1f(heightLoc, cameraHeight);
		GLint yawLoc = glGetUniformLocation(program_->GetId(), "u_CameraYaw");
		glUniform1f(yawLoc, cameraYaw);
		GLint pitchLoc = glGetUniformLocation(program_->GetId(), "u_CameraPitch");
		glUniform1f(pitchLoc, cameraPitch);
		GLint fovLoc = glGetUniformLocation(program_->GetId(), "u_CameraFOV");
		glUniform1f(fovLoc, cameraFOV);
		GLint sizeLoc = glGetUniformLocation(program_->GetId(), "u_TrackSize");
		glUniform2f(sizeLoc, trackSize_.x, trackSize_.y);


		glActiveTexture(GL_TEXTURE0);
		trackTexture_->Bind(0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glActiveTexture(GL_TEXTURE1);
		outOfBoundsTexture_->Bind(1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glDisable(GL_DEPTH_TEST);
		vertexArray_->Bind();
		vertexArray_->DrawTriangles();
		glEnable(GL_DEPTH_TEST);
	}

	void BakedSurfaceRenderer::BakeObject(
		SDL_Surface* surface, std::vector<glm::vec2> positions)
	{
		if (!surface) return;

		for (const auto& pos : positions) 
		{
			SDL_Rect destRect;
			destRect.x = static_cast<int>(pos.x);
			destRect.y = static_cast<int>(pos.y);
			destRect.w = surface->w;
			destRect.h = surface->h;

			SDL_BlitSurface(surface, nullptr, workingTrackSurface_, &destRect);
		}

		isTrackTextureDirty_ = true;
	}
	void BakedSurfaceRenderer::BakeRotatedObjects(
		SDL_Surface* surface, const std::vector<RotatedBakeItem>& items)
	{
		if (!surface) return;

		SDL_Surface* srcRGBA = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
		if (!srcRGBA) return;

		int srcW = srcRGBA->w;
		int srcH = srcRGBA->h;

		for (const auto& item : items)
		{
			// 1. 確保方向在 0~3 之間安全循環
			int dir = item.direction % 4;
			if (dir < 0) dir += 4;

			// 2. 如果是轉 90 度或 270 度，目標畫布的寬高必須互換！
			int destW = (dir == 1 || dir == 3) ? srcH : srcW;
			int destH = (dir == 1 || dir == 3) ? srcW : srcH;

			SDL_Surface* rotatedSurface = SDL_CreateRGBSurfaceWithFormat(
				0, destW, destH, 32, SDL_PIXELFORMAT_RGBA32);

			// 填滿透明底
			SDL_FillRect(rotatedSurface, nullptr, SDL_MapRGBA(rotatedSurface->format, 0, 0, 0, 0));

			// 3. 🌟 超高速純整數像素映射 (無三角函數)
			for (int dy = 0; dy < destH; ++dy)
			{
				Uint32* destRow = (Uint32*)((Uint8*)rotatedSurface->pixels + dy * rotatedSurface->pitch);

				for (int dx = 0; dx < destW; ++dx)
				{
					int srcX = 0, srcY = 0;

					// 根據方向，直接反推原始圖片的 X, Y 座標
					switch (dir)
					{
					case 0: // 正常
						srcX = dx;
						srcY = dy;
						break;
					case 1: // 逆時鐘 90 度
						srcX = srcW - 1 - dy;
						srcY = dx;
						break;
					case 2: // 180 度 (上下左右顛倒)
						srcX = srcW - 1 - dx;
						srcY = srcH - 1 - dy;
						break;
					case 3: // 順時鐘 90 度
						srcX = dy;
						srcY = srcH - 1 - dx;
						break;
					}

					// 直接拷貝顏色 (保證 100% 不破圖)
					Uint32* srcRow = (Uint32*)((Uint8*)srcRGBA->pixels + srcY * srcRGBA->pitch);
					destRow[dx] = srcRow[srcX];
				}
			}

			// 4. 計算中心點並蓋印到賽道上
			SDL_Rect destRect;
			destRect.x = static_cast<int>(item.position.x);
			destRect.y = static_cast<int>(item.position.y);
			destRect.w = destW;
			destRect.h = destH;

			SDL_BlitSurface(rotatedSurface, nullptr, workingTrackSurface_, &destRect);
			SDL_FreeSurface(rotatedSurface);
		}

		SDL_FreeSurface(srcRGBA);
		isTrackTextureDirty_ = true;
	}
	void BakedSurfaceRenderer::RestoreBakedObject(
		glm::vec2 size, std::vector<glm::vec2> positions)
	{
		for (const auto& pos : positions)
		{
			SDL_Rect patchRect;
			patchRect.x = static_cast<int>(pos.x);
			patchRect.y = static_cast<int>(pos.y);
			patchRect.w = size.x;
			patchRect.h = size.y;

			SDL_BlitSurface(cleanTrackSurface_, &patchRect, workingTrackSurface_, &patchRect);
		}

		isTrackTextureDirty_ = true;
	}
}
