#pragma once

// 視窗布局格式
namespace Game::GameCore
{
	struct RetroLayout
	{
		int width = 256;
		int height = 224;

		int topBar = 3;
		int sky = 21;
		int track = 83;
		int midBar = 8;
		int map = 104;
		int bottomBar = 5;

		float pixelScale = 2.0f;

		int GetMapY() const { return bottomBar; }
		int GetTrackY() const { return GetMapY() + map + midBar; }
		int GetSkyY() const { return GetTrackY() + track; }
	};

	static RetroLayout RETRO_LAYOUT;
}
