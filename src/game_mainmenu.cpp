#pragma once

void updateAndRenderMainMenu(GameState *gameState, InputState *inputState, Renderer *renderer, AssetManager *assets)
{
	gameState->status = updateAndRenderMainMenuUI(renderer, assets, &gameState->uiState, inputState, gameState->status);
}
