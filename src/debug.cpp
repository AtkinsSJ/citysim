#pragma once

void processDebugData(DebugState *debugState)
{
	if (debugState)
	{
		debugState->readingFrameIndex = debugState->writingFrameIndex;
		debugState->writingFrameIndex = (debugState->writingFrameIndex + 1) % DEBUG_FRAMES_COUNT;

		// Free old top blocks list

		// Calculate new top blocks list
		DebugCodeData *code = debugState->codeDataSentinel.next;
		while (code != &debugState->codeDataSentinel)
		{
			

			code = code->next;
		}

		// Zero-out new writing frame.
		DebugCodeData *codeData = debugState->codeDataSentinel.next;
		uint32 wfi = debugState->writingFrameIndex;
		while (codeData != &debugState->codeDataSentinel)
		{
			codeData->callCount[wfi] = 0;
			codeData->totalCycleCount[wfi] = 0;
			codeData->averageCycleCount[wfi] = 0;
			codeData = codeData->next;
		}
	}
}

void renderDebugData(DebugState *debugState, UIState *uiState, Renderer *renderer)
{
	if (debugState)
	{
		uint32 frameIndex = debugState->readingFrameIndex;
		real32 screenEdgePadding = 16.0f;
		V2 textPos = v2(screenEdgePadding, screenEdgePadding);
		char stringBuffer[1024];
		BitmapFont *font = debugState->font;
		V4 textColor = makeWhite();
		real32 maxWidth = renderer->uiBuffer.camera.size.x - (2*screenEdgePadding);

		drawRect(&renderer->uiBuffer, rectXYWH(0,0,renderer->uiBuffer.camera.size.x, renderer->uiBuffer.camera.size.y),
			     99, color255(0,0,0,128));

		DebugArenaData *arena = debugState->arenaDataSentinel.next;
		while (arena != &debugState->arenaDataSentinel)
		{
			sprintf(stringBuffer, "Memory arena %s: %d blocks, %" PRIuPTR " used / %" PRIuPTR " allocated",
				    arena->name, arena->blockCount[frameIndex], arena->usedSize[frameIndex], arena->totalSize[frameIndex]);
			textPos.y += uiText(uiState, renderer, font, stringBuffer, textPos, ALIGN_LEFT, 100, textColor, maxWidth).h;
			arena = arena->next;
		}

		uint64 cyclesPerSecond = SDL_GetPerformanceFrequency();
		sprintf(stringBuffer, "There are %" PRIu64 " cycles in a second", cyclesPerSecond);
		textPos.y += uiText(uiState, renderer, font, stringBuffer, textPos, ALIGN_LEFT, 100, textColor, maxWidth).h;

		DebugCodeData *code = debugState->codeDataSentinel.next;
		while (code != &debugState->codeDataSentinel)
		{
			sprintf(stringBuffer, "Code '%s' called %d times, %" PRIu64 " cycles, avg %" PRIu64 " cycles",
				    code->name, code->callCount[frameIndex], code->totalCycleCount[frameIndex],
				    code->averageCycleCount[frameIndex]);
			textPos.y += uiText(uiState, renderer, font, stringBuffer, textPos, ALIGN_LEFT, 100, textColor, maxWidth).h;
			code = code->next;
		}
	}
}