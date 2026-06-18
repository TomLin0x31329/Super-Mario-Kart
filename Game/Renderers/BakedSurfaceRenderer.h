#pragma once

#include "pch.hpp"

#include "Core/Program.hpp"
#include "Core/VertexArray.hpp"
#include "Core/Texture.hpp"

namespace Game::Renderer
{
	struct RotatedBakeItem
	{
		glm::vec2 position;
		int direction;
	};

	class BakedSurfaceRenderer
	{
	private:
		std::unique_ptr<Core::Program> program_;
		std::unique_ptr<Core::VertexArray> vertexArray_;

		std::unique_ptr<Core::Texture> trackTexture_;
		std::unique_ptr<Core::Texture> outOfBoundsTexture_;
		glm::vec2 trackSize_{};
		bool isTrackTextureDirty_ = true;

		SDL_Surface* cleanTrackSurface_ = nullptr;
		SDL_Surface* workingTrackSurface_ = nullptr;

		const float TRACK_SCALE = 150.0f;
		const float UV_START = -4.0f;
		const float UV_END = 5.0f;
		const float UV_SPAN = UV_END - UV_START;
		const float MAP_PIXEL_SIZE = 1024.0f;
		const float MAP_WORLD_SIZE = 33.333333f;
		const float PIXEL_TO_WORLD_RATIO = MAP_PIXEL_SIZE / MAP_WORLD_SIZE;

		//glm::vec3 cameraPos_ = { 14.32, 0.5f, 4.22873 };

	private:
		void UpdateGLTexture();

	public:
		BakedSurfaceRenderer() = default;
		BakedSurfaceRenderer(const BakedSurfaceRenderer&) = delete;
		BakedSurfaceRenderer& operator=(const BakedSurfaceRenderer&) = delete;
		~BakedSurfaceRenderer();

	public:
		bool Init(
			const std::string& trackTexturePath,
			const std::string& outOfBoundsTexturePath);
		void Draw(
			glm::vec2 cameraPos, float cameraYaw, 
			float cameraPitch, float cameraFOV, float cameraHeight);

		void BakeObject(
			SDL_Surface* surface, std::vector<glm::vec2> positions);
		void BakeRotatedObjects(
			SDL_Surface* surface, const std::vector<RotatedBakeItem>& items);
		void RestoreBakedObject(
			glm::vec2 size, std::vector<glm::vec2> positions);

	};
}
