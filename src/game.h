#pragma once

#include "city.h"

const int gameStartFunds = 10000;
const int gameWinFunds = 30000;

enum GameStatus
{
	GameStatus_Setup,
	GameStatus_Playing,
	GameStatus_Won,
	GameStatus_Lost,
	GameStatus_Quit,
};

struct GameState
{
	MemoryArena gameArena;
	GameStatus status;
	City city;
};