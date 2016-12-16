#pragma once

#include "city.h"

const int gameStartFunds = 10000;
const int gameWinFunds = 30000;

struct GameState
{
	MemoryArena gameArena;
	GameStatus status;
	City city;
};