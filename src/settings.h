#pragma once

struct Settings
{
	bool windowed;
	s32 windowWidth;
	s32 windowHeight;
};

void loadSettings(Settings *settings, struct AssetManager *assets, File file);
void applySettings(Settings *settings);