#include "LevelLoader.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include "Util/Logger.hpp"


namespace Game::Systems
{
	Game::GameCore::LevelData LevelLoader::LoadLevel(const std::string& levelPath)
	{
		Game::GameCore::LevelData data;
		std::ifstream file(levelPath);

		if (!file.is_open())
		{
			LOG_ERROR("Cannot open JSON: {}", levelPath);
			return data;
		}
		nlohmann::json j;
		file >> j;

		data.name = j["meta"]["name"];
		data.totalLaps = j["meta"]["total_laps"];
		data.initialYaw = j["starting_grid"]["initial_yaw"];
		for (const auto& pos : j["starting_grid"]["positions"])
		{
			data.startingGrid.push_back({ pos["id"], {pos["x"], pos["z"]} });
		}
		if (j.contains("coin"))
		{
			for (const auto& pos : j["coin"]["positions"])
			{
				data.coins.push_back({ pos["x"], pos["z"] });
			}
		}
		if (j.contains("ITEM_BOX"))
		{
			for (const auto& box : j["ITEM_BOX"]["positions"])
			{
				data.itemBoxes.push_back({ { box["x"], box["z"] }, box["direction"]});
			}
		}
		if (j.contains("pipe"))
		{
			for (const auto& pos : j["pipe"]["positions"])
			{
				data.pipes.push_back({ pos["x"], pos["z"] });
			}
		}

		std::string collisionImagePath = std::string(RESOURCE_DIR) + "/sprites/tracks/level_1/collision.png";
		SDL_Surface* rawSurface = IMG_Load(collisionImagePath.c_str());

		if (rawSurface)
		{
			SDL_Surface* rgbaSurface = SDL_ConvertSurfaceFormat(rawSurface, SDL_PIXELFORMAT_RGBA32, 0);
			if (rgbaSurface)
			{
				data.collisionMap.width = rgbaSurface->w;
				data.collisionMap.height = rgbaSurface->h;

				// 計算總位元組數量 (寬 * 高 * 4 bytes)
				int bufferSize = rgbaSurface->w * rgbaSurface->h * 4;
				data.collisionMap.pixels.resize(bufferSize);

				// 將圖片像素一次性拷貝到 vector 中
				std::memcpy(data.collisionMap.pixels.data(), rgbaSurface->pixels, bufferSize);

				SDL_FreeSurface(rgbaSurface);
				LOG_INFO("Successfully loaded collision map into RAM.");
			}
			SDL_FreeSurface(rawSurface);
		}
		else
		{
			LOG_ERROR("Failed to load collision map: {}", IMG_GetError());
		}

		return data;
	}
}
