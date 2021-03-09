#pragma once

struct AppState
{
	AppStatus appStatus;
	MemoryArena systemArena;

	UIState *uiState;
	GameState *gameState;
	Random cosmeticRandom; // Appropriate for when you need a random number and don't care if it's consistent!

	f32 rawDeltaTime;
	f32 speedMultiplier;
	f32 deltaTime;

	inline void setDeltaTimeFromLastFrame(f32 lastFrameTime)
	{
		rawDeltaTime = lastFrameTime;
		deltaTime = rawDeltaTime * speedMultiplier;
	}

	inline void setSpeedMultiplier(f32 multiplier)
	{
		speedMultiplier = multiplier;
		deltaTime = rawDeltaTime * speedMultiplier;
	}
};
