#pragma once

#include "city.h"

const int gameStartFunds = 1000000;

enum GameStatus
{
	GameStatus_Playing,
	GameStatus_Quit,
};

struct GameState
{
	MemoryArena gameArena;
	GameStatus status;
	Random gameRandom;
	City city;

	u32 dataLayerToDraw;
};