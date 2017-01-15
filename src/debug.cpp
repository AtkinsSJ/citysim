#pragma once

void processDebugData(DebugState *debugState)
{
	DEBUG_FUNCTION();
	if (debugState)
	{
		debugState->readingFrameIndex = debugState->writingFrameIndex;
		debugState->writingFrameIndex = (debugState->writingFrameIndex + 1) % DEBUG_FRAMES_COUNT;

		DLinkedListFreeAll(DebugCodeDataWrapper, &debugState->topCodeBlocksSentinel,
			               &debugState->topCodeBlocksFreeListSentinel);
		ASSERT(DLinkedListIsEmpty(&debugState->topCodeBlocksSentinel), "List we just freed is not empty!");

		// Calculate new top blocks list
		DebugCodeData *code = debugState->codeDataSentinel.next;
		while (code != &debugState->codeDataSentinel)
		{
			// Find spot on list. If we fail, target is the sentinel so it all still works!
			DebugCodeDataWrapper *target = debugState->topCodeBlocksSentinel.next;
			bool foundSmallerItem = false;
			while (target != &debugState->topCodeBlocksSentinel)
			{
				if (target->data->totalCycleCount[debugState->readingFrameIndex]
				    < code->totalCycleCount[debugState->readingFrameIndex])
				{
					foundSmallerItem = true;
					break;
				}
				target = target->next;
			}

			bool freeListHasItem = !DLinkedListIsEmpty(&debugState->topCodeBlocksFreeListSentinel);

			DebugCodeDataWrapper *item = 0;
			if (freeListHasItem)
			{
				item = debugState->topCodeBlocksFreeListSentinel.next;
				DLinkedListRemove(item);
			}
			else if (foundSmallerItem)
			{
				item = debugState->topCodeBlocksSentinel.prev;
				DLinkedListRemove(item);
			}

			if (item)
			{
				item->data = code;
				DLinkedListInsertBefore(item, target);
			}

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

void renderDebugData(DebugState *debugState, UIState *uiState, RenderBuffer *uiBuffer)
{
	if (debugState)
	{
		int32 frameIndex = debugState->readingFrameIndex;
		real32 screenEdgePadding = 16.0f;
		V2 textPos = v2(screenEdgePadding, screenEdgePadding);
		char stringBuffer[1024];
		BitmapFont *font = debugState->font;
		V4 textColor = makeWhite();
		real32 maxWidth = uiBuffer->camera.size.x - (2*screenEdgePadding);

		drawRect(uiBuffer, rectXYWH(0,0,uiBuffer->camera.size.x, uiBuffer->camera.size.y),
			     99, color255(0,0,0,128));

		sprintf(stringBuffer, "Examing %d frames ago",
			    ((debugState->writingFrameIndex - frameIndex) + DEBUG_FRAMES_COUNT) % DEBUG_FRAMES_COUNT);
		textPos.y += uiText(uiState, uiBuffer, font, stringBuffer, textPos, ALIGN_LEFT, 100, textColor, maxWidth).h;

		DebugArenaData *arena = debugState->arenaDataSentinel.next;
		while (arena != &debugState->arenaDataSentinel)
		{
			sprintf(stringBuffer, "Memory arena %s: %d blocks, %" PRIuPTR " used / %" PRIuPTR " allocated",
				    arena->name, arena->blockCount[frameIndex], arena->usedSize[frameIndex], arena->totalSize[frameIndex]);
			textPos.y += uiText(uiState, uiBuffer, font, stringBuffer, textPos, ALIGN_LEFT, 100, textColor, maxWidth).h;
			arena = arena->next;
		}

		uint64 cyclesPerSecond = SDL_GetPerformanceFrequency();
		sprintf(stringBuffer, "There are %" PRIu64 " cycles in a second", cyclesPerSecond);
		textPos.y += uiText(uiState, uiBuffer, font, stringBuffer, textPos, ALIGN_LEFT, 100, textColor, maxWidth).h;
		int32 width = sprintf(stringBuffer, "%30s| %20s| %20s| %20s", "Code", "Total cycles", "Calls", "Avg Cycles");
		textPos.y += uiText(uiState, uiBuffer, font, stringBuffer, textPos, ALIGN_LEFT, 100, textColor, maxWidth).h;

		char line[] = "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------";
		sprintf(stringBuffer, "%.*s", width, line);
		textPos.y += uiText(uiState, uiBuffer, font, stringBuffer, textPos, ALIGN_LEFT, 100, textColor, maxWidth).h;

		DebugCodeDataWrapper *topBlock = debugState->topCodeBlocksSentinel.next;
		while (topBlock != &debugState->topCodeBlocksSentinel)
		{
			DebugCodeData *code = topBlock->data;
			sprintf(stringBuffer, "%30s| %20" PRIu64 "| %20d| %20" PRIu64 "",
				    code->name, code->totalCycleCount[frameIndex],
				    code->callCount[frameIndex], code->averageCycleCount[frameIndex]);
			textPos.y += uiText(uiState, uiBuffer, font, stringBuffer, textPos, ALIGN_LEFT, 100, textColor, maxWidth).h;
			topBlock = topBlock->next;
		}
	}
}

void debugUpdate(DebugState *debugState, InputState *inputState, UIState *uiState, RenderBuffer *uiBuffer)
{
	if (keyJustPressed(inputState, SDLK_F2))
	{
		debugState->showDebugData = !debugState->showDebugData;
	}
	
	if (keyJustPressed(inputState, SDLK_PAUSE, KeyMod_Shift))
	{
		debugState->captureDebugData = !debugState->captureDebugData;
	}

	if (keyJustPressed(inputState, SDLK_KP_MINUS, KeyMod_Shift))
	{
		debugState->readingFrameIndex = (debugState->readingFrameIndex - 1 + DEBUG_FRAMES_COUNT) % DEBUG_FRAMES_COUNT;
	}
	else if (keyJustPressed(inputState, SDLK_KP_PLUS, KeyMod_Shift))
	{
		debugState->readingFrameIndex = (debugState->readingFrameIndex + 1 + DEBUG_FRAMES_COUNT) % DEBUG_FRAMES_COUNT;
	}

	if (debugState->captureDebugData)
	{
		processDebugData(debugState);
	}

	if (debugState->showDebugData)
	{
		renderDebugData(debugState, uiState, uiBuffer);
	}
}