#pragma once
#include <stdarg.h>

void clearDebugFrame(DebugState *debugState, int32 frameIndex)
{
	DebugCodeData *codeData = debugState->codeDataSentinel.next;
	while (codeData != &debugState->codeDataSentinel)
	{
		codeData->callCount[frameIndex] = 0;
		codeData->totalCycleCount[frameIndex] = 0;
		codeData->averageCycleCount[frameIndex] = 0;
		codeData = codeData->next;
	}
}

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
		clearDebugFrame(debugState, debugState->writingFrameIndex);
	}
}

struct DebugTextState
{
	V2 pos;
	char buffer[1024];
	BitmapFont *font;
	V4 color;
	real32 maxWidth;

	UIState *uiState;
	RenderBuffer *uiBuffer;

	int32 charsLastPrinted;
};
inline DebugTextState initDebugTextState(UIState *uiState, RenderBuffer *uiBuffer, BitmapFont *font, V4 textColor,
	                                     real32 screenWidth, real32 screenEdgePadding)
{
	DebugTextState textState = {};
	textState.pos = v2(screenEdgePadding, screenEdgePadding);
	textState.font = font;
	textState.color = textColor;
	textState.maxWidth = screenWidth - (2*screenEdgePadding);

	textState.uiState = uiState;
	textState.uiBuffer = uiBuffer;

	return textState;
}
void debugTextOut(DebugTextState *textState, const char *formatString, ...)
{
	va_list args;
	va_start(args, formatString);
	textState->charsLastPrinted = vsnprintf(textState->buffer, sizeof(textState->buffer), formatString, args);
	RealRect resultRect = uiText(textState->uiState, textState->uiBuffer, textState->font, textState->buffer, textState->pos,
	                             ALIGN_LEFT, 100, textState->color, textState->maxWidth);
	textState->pos.y += resultRect.h;
	va_end(args);
}

void renderDebugData(DebugState *debugState, UIState *uiState, RenderBuffer *uiBuffer)
{
	if (debugState)
	{
		int32 frameIndex = debugState->readingFrameIndex;
		drawRect(uiBuffer, rectXYWH(0,0,uiBuffer->camera.size.x, uiBuffer->camera.size.y),
			     99, color255(0,0,0,128));

		DebugTextState textState = initDebugTextState(uiState, uiBuffer, debugState->font, makeWhite(),
			                                          uiBuffer->camera.size.x, 16.0f);

		debugTextOut(&textState, "Examing %d frames ago",
			    ((debugState->writingFrameIndex - frameIndex) + DEBUG_FRAMES_COUNT) % DEBUG_FRAMES_COUNT);

		DebugArenaData *arena = debugState->arenaDataSentinel.next;
		while (arena != &debugState->arenaDataSentinel)
		{
			debugTextOut(&textState, "Memory arena %s: %d blocks, %" PRIuPTR " used / %" PRIuPTR " allocated",
				    arena->name, arena->blockCount[frameIndex], arena->usedSize[frameIndex], arena->totalSize[frameIndex]);
			arena = arena->next;
		}

		uint64 cyclesPerSecond = SDL_GetPerformanceFrequency();
		debugTextOut(&textState, "There are %" PRIu64 " cycles in a second", cyclesPerSecond);
		debugTextOut(&textState, "%30s| %20s| %20s| %20s", "Code", "Total cycles", "Calls", "Avg Cycles");

		char line[] = "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------";
		ASSERT(strlen(line) >= textState.charsLastPrinted,
			   "line not long enough! Length is %d, needed %d", strlen(line), textState.charsLastPrinted);
		debugTextOut(&textState, "%.*s", textState.charsLastPrinted, line);

		DebugCodeDataWrapper *topBlock = debugState->topCodeBlocksSentinel.next;
		while (topBlock != &debugState->topCodeBlocksSentinel)
		{
			DebugCodeData *code = topBlock->data;
			debugTextOut(&textState, "%30s| %20" PRIu64 "| %20d| %20" PRIu64 "",
				         code->name, code->totalCycleCount[frameIndex],
				         code->callCount[frameIndex], code->averageCycleCount[frameIndex]);
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
	else
	{
		// Clear current frame to prevent all the incoming events building up.
		// (We still receive and log events when paused, we just don't progress.)
		// So, we technically lose information here but oh well. When we pause debug info, it's to examine the recent past,
		// not monitor what's happening *now*.
		clearDebugFrame(debugState, debugState->writingFrameIndex);
	}

	if (debugState->showDebugData)
	{
		renderDebugData(debugState, uiState, uiBuffer);
	}
}