#pragma once
#include <stdarg.h>

void debugInit(BitmapFont *font)
{
	bootstrapArena(DebugState, globalDebugState, debugArena);
	globalDebugState->showDebugData = true;
	globalDebugState->captureDebugData = true;
	globalDebugState->readingFrameIndex = DEBUG_FRAMES_COUNT - 1;
	globalDebugState->font = font;

	DLinkedListInit(&globalDebugState->arenaDataSentinel);
	DLinkedListInit(&globalDebugState->codeDataSentinel);

	DLinkedListInit(&globalDebugState->topCodeBlocksFreeListSentinel);
	DLinkedListInit(&globalDebugState->topCodeBlocksSentinel);
	for (u32 i=0; i<DEBUG_TOP_CODE_BLOCKS_COUNT; i++)
	{
		DebugCodeDataWrapper *item = PushStruct(&globalDebugState->debugArena, DebugCodeDataWrapper);
		DLinkedListInsertBefore(item, &globalDebugState->topCodeBlocksFreeListSentinel);
	}
}

void clearDebugFrame(DebugState *debugState, s32 frameIndex)
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
		debugState->frameEndCycle[debugState->writingFrameIndex] = SDL_GetPerformanceCounter();
		if (debugState->captureDebugData)
		{
			debugState->readingFrameIndex = debugState->writingFrameIndex;
			debugState->writingFrameIndex = (debugState->writingFrameIndex + 1) % DEBUG_FRAMES_COUNT;
		}
		debugState->frameStartCycle[debugState->writingFrameIndex] = SDL_GetPerformanceCounter();

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
	f32 maxWidth;
	bool progressUpwards;

	UIState *uiState;
	RenderBuffer *uiBuffer;

	u32 charsLastPrinted;
};
inline DebugTextState initDebugTextState(UIState *uiState, RenderBuffer *uiBuffer, BitmapFont *font, V4 textColor,
	                                     V2 screenSize, f32 screenEdgePadding, bool upwards)
{
	DebugTextState textState = {};
	textState.progressUpwards = upwards;
	if (upwards) 
	{
		textState.pos = v2(screenEdgePadding, screenSize.y - screenEdgePadding);
	}
	else
	{
		textState.pos = v2(screenEdgePadding, screenEdgePadding);
	}
	textState.font = font;
	textState.color = textColor;
	textState.maxWidth = screenSize.x - (2*screenEdgePadding);

	textState.uiState = uiState;
	textState.uiBuffer = uiBuffer;

	return textState;
}

void debugTextOut(DebugTextState *textState, String text)
{
	s32 align = ALIGN_LEFT;
	if (textState->progressUpwards) align |= ALIGN_BOTTOM;
	else                            align |= ALIGN_TOP;

	textState->charsLastPrinted = text.length;
	RealRect resultRect = uiText(textState->uiState, textState->uiBuffer, textState->font, text, textState->pos,
	                             align, 300, textState->color, textState->maxWidth);

	if (textState->progressUpwards)
	{
		textState->pos.y -= resultRect.h;
	}
	else
	{
		textState->pos.y += resultRect.h;
	}
}

void renderDebugData(DebugState *debugState, UIState *uiState, RenderBuffer *uiBuffer)
{
	if (debugState)
	{
		u32 rfi = debugState->readingFrameIndex;
		drawRect(uiBuffer, rectXYWH(0,0,uiBuffer->camera.size.x, uiBuffer->camera.size.y),
			     100, color255(0,0,0,128));

		DebugTextState textState = initDebugTextState(uiState, uiBuffer, debugState->font, makeWhite(),
			                                          uiBuffer->camera.size, 16.0f, false);

		u32 framesAgo = WRAP(debugState->writingFrameIndex - rfi, DEBUG_FRAMES_COUNT);
		debugTextOut(&textState, myprintf("Examing {0} frames ago", {formatInt(framesAgo)}));

		DebugArenaData *arena = debugState->arenaDataSentinel.next;
		while (arena != &debugState->arenaDataSentinel)
		{
			debugTextOut(&textState, myprintf("Memory arena {0}: {1} blocks, {2} used / {3} allocated",
				{arena->name, formatInt(arena->blockCount[rfi]), formatInt(arena->usedSize[rfi]), formatInt(arena->totalSize[rfi])}));
			arena = arena->next;
		}

		u64 cyclesPerSecond = SDL_GetPerformanceFrequency();
		debugTextOut(&textState, myprintf("There are {0} cycles in a second", {formatInt(cyclesPerSecond)}));
		debugTextOut(&textState, myprintf("{0}| {1}| {2}| {3}",
			{formatString("Code", 30), formatString("Total cycles", 20, false),
			 formatString("Calls", 20, false), formatString("Avg Cycles", 20, false)}));

		debugTextOut(&textState, repeatChar('-', textState.charsLastPrinted));

		DebugCodeDataWrapper *topBlock = debugState->topCodeBlocksSentinel.next;
		while (topBlock != &debugState->topCodeBlocksSentinel)
		{
			DebugCodeData *code = topBlock->data;
			debugTextOut(&textState, myprintf("{0}| {1}| {2}| {3}",
				{formatString(code->name, 30),
				 formatString(formatInt(code->totalCycleCount[rfi]), 20, false),
				 formatString(formatInt(code->callCount[rfi]), 20, false),
				 formatString(formatInt(code->averageCycleCount[rfi]), 20, false)}));
			topBlock = topBlock->next;
		}

		// Draw a nice chart!
		f32 graphHeight = 150.0f;
		f32 targetCyclesPerFrame = cyclesPerSecond / 60.0f;
		f32 barWidth = uiBuffer->camera.size.x / (f32)DEBUG_FRAMES_COUNT;
		f32 barHeightPerCycle = graphHeight / targetCyclesPerFrame;
		V4 barColor = color255(255, 0, 0, 128);
		V4 activeBarColor = color255(255, 255, 0, 128);
		u32 barIndex = 0;
		for (u32 fi = debugState->writingFrameIndex + 1;
			 fi != debugState->writingFrameIndex;
			 fi = WRAP(fi + 1, DEBUG_FRAMES_COUNT))
		{
			u64 frameCycles = debugState->frameEndCycle[fi] - debugState->frameStartCycle[fi];
			f32 barHeight = barHeightPerCycle * (f32)frameCycles;
			drawRect(uiBuffer, rectXYWH(barWidth * barIndex++, uiBuffer->camera.size.y - barHeight, barWidth, barHeight), 200,
				     fi == rfi ? activeBarColor : barColor);
		}
		drawRect(uiBuffer, rectXYWH(0, uiBuffer->camera.size.y - graphHeight, uiBuffer->camera.size.x, 1),
		         201, color255(255, 255, 255, 128));
		drawRect(uiBuffer, rectXYWH(0, uiBuffer->camera.size.y - graphHeight*2, uiBuffer->camera.size.x, 1),
		         201, color255(255, 255, 255, 128));
	}
}


void debugUpdate(DebugState *debugState, InputState *inputState, UIState *uiState, RenderBuffer *uiBuffer)
{
	if (keyJustPressed(inputState, SDLK_F2))
	{
		debugState->showDebugData = !debugState->showDebugData;
	}
	
	if (keyJustPressed(inputState, SDLK_PAUSE))
	{
		debugState->captureDebugData = !debugState->captureDebugData;
	}

	if (keyJustPressed(inputState, SDLK_PAGEDOWN))
	{
		debugState->readingFrameIndex = WRAP(debugState->readingFrameIndex - 1, DEBUG_FRAMES_COUNT);
	}
	else if (keyJustPressed(inputState, SDLK_PAGEUP))
	{
		debugState->readingFrameIndex = WRAP(debugState->readingFrameIndex + 1, DEBUG_FRAMES_COUNT);
	}

	processDebugData(debugState);

	if (debugState->showDebugData)
	{
		renderDebugData(debugState, uiState, uiBuffer);
	}
}