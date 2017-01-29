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
	real32 maxWidth;
	bool progressUpwards;

	UIState *uiState;
	RenderBuffer *uiBuffer;

	uint32 charsLastPrinted;
};
inline DebugTextState initDebugTextState(UIState *uiState, RenderBuffer *uiBuffer, BitmapFont *font, V4 textColor,
	                                     V2 screenSize, real32 screenEdgePadding, bool upwards)
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
void debugTextOut(DebugTextState *textState, const char *formatString, ...)
{
	va_list args;
	va_start(args, formatString);

	int32 align = ALIGN_LEFT;
	if (textState->progressUpwards) align |= ALIGN_BOTTOM;
	else                            align |= ALIGN_TOP;

	textState->charsLastPrinted = vsnprintf(textState->buffer, sizeof(textState->buffer), formatString, args);
	RealRect resultRect = uiText(textState->uiState, textState->uiBuffer, textState->font, textState->buffer, textState->pos,
	                             align, 300, textState->color, textState->maxWidth);
	if (textState->progressUpwards)
	{
		textState->pos.y -= resultRect.h;
	}
	else
	{
		textState->pos.y += resultRect.h;
	}
	va_end(args);
}

void renderDebugConsole(DebugState *debugState, DebugConsole *console, UIState *uiState, RenderBuffer *uiBuffer)
{
	DebugTextState textState = initDebugTextState(uiState, uiBuffer, debugState->font, makeWhite(),
		                                          uiBuffer->camera.size, 16.0f, true);

	// Caret stuff is a bit hacky, but oh well.
	// It especially doesn't play well with pausing the debug capturing...
	char caret = '_';
	if ((debugState->readingFrameIndex % FRAMES_PER_SECOND) < (FRAMES_PER_SECOND/2))
	{
		caret = ' ';
	}
	debugTextOut(&textState, "> %.*s%c", console->input.bufferLength, console->input.buffer, caret);

	// print output lines
	for (int32 i=console->outputLineCount-1; i>=0; i--)
	{
		StringBuffer *line = console->outputLines + WRAP(console->currentOutputLine + i, console->outputLineCount);
		debugTextOut(&textState, "%s", line->buffer);
	}
}

void renderDebugData(DebugState *debugState, UIState *uiState, RenderBuffer *uiBuffer)
{
	if (debugState)
	{
		uint32 rfi = debugState->readingFrameIndex;
		drawRect(uiBuffer, rectXYWH(0,0,uiBuffer->camera.size.x, uiBuffer->camera.size.y),
			     100, color255(0,0,0,128));

		DebugTextState textState = initDebugTextState(uiState, uiBuffer, debugState->font, makeWhite(),
			                                          uiBuffer->camera.size, 16.0f, false);

		debugTextOut(&textState, "Examing %d frames ago", WRAP(debugState->writingFrameIndex - rfi, DEBUG_FRAMES_COUNT));

		DebugArenaData *arena = debugState->arenaDataSentinel.next;
		while (arena != &debugState->arenaDataSentinel)
		{
			debugTextOut(&textState, "Memory arena %s: %d blocks, %" PRIuPTR " used / %" PRIuPTR " allocated",
				    arena->name, arena->blockCount[rfi], arena->usedSize[rfi], arena->totalSize[rfi]);
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
				         code->name, code->totalCycleCount[rfi],
				         code->callCount[rfi], code->averageCycleCount[rfi]);
			topBlock = topBlock->next;
		}

		// Draw a nice chart!
		real32 graphHeight = 150.0f;
		real32 targetCyclesPerFrame = cyclesPerSecond / 60.0f;
		real32 barWidth = uiBuffer->camera.size.x / (real32)DEBUG_FRAMES_COUNT;
		real32 barHeightPerCycle = graphHeight / targetCyclesPerFrame;
		V4 barColor = color255(255, 0, 0, 128);
		V4 activeBarColor = color255(255, 255, 0, 128);
		uint32 barIndex = 0;
		for (uint32 fi = debugState->writingFrameIndex + 1;
			 fi != debugState->writingFrameIndex;
			 fi = WRAP(fi + 1, DEBUG_FRAMES_COUNT))
		{
			uint64 frameCycles = debugState->frameEndCycle[fi] - debugState->frameStartCycle[fi];
			real32 barHeight = barHeightPerCycle * (real32)frameCycles;
			drawRect(uiBuffer, rectXYWH(barWidth * barIndex++, uiBuffer->camera.size.y - barHeight, barWidth, barHeight), 200,
				     fi == rfi ? activeBarColor : barColor);
		}
		drawRect(uiBuffer, rectXYWH(0, uiBuffer->camera.size.y - graphHeight, uiBuffer->camera.size.x, 1),
		         201, color255(255, 255, 255, 128));
		drawRect(uiBuffer, rectXYWH(0, uiBuffer->camera.size.y - graphHeight*2, uiBuffer->camera.size.x, 1),
		         201, color255(255, 255, 255, 128));
	}
}

StringBuffer *debugConsoleNextOutputLine(DebugConsole *console)
{
	StringBuffer *result = console->outputLines + console->currentOutputLine;
	console->currentOutputLine = WRAP(console->currentOutputLine + 1, console->outputLineCount);

	clear(result);

	return result;
}

void debugHandleConsoleInput(DebugConsole *console)
{
	// copy input to output, for readability
	{
		StringBuffer *output = debugConsoleNextOutputLine(console);
		append(output, "> ");
		append(output, &console->input);
	}

	if (console->input.bufferLength == 0)
	{
		// Ignore blank lines
	}
	else if (equals(&console->input, "help"))
	{
		append(debugConsoleNextOutputLine(console), "The only command is 'help'. I admit this is not very useful.");
	}
	else
	{
		StringBuffer *output = debugConsoleNextOutputLine(console);
		append(output, "I don't understand '");
		append(output, &console->input);
		append(output, "'. Try 'help' for a list of commands.");
	}

	// Do this last so we can actually read the input. To do otherwise would be Very Dumb™.
	clear(&console->input);
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
		DebugConsole *console = &debugState->console;

		if (keyJustPressed(inputState, SDLK_BACKSPACE, KeyMod_Shift))
		{
			clear(&console->input);
		}
		else if (keyJustPressed(inputState, SDLK_BACKSPACE))
		{
			backspace(&console->input);
		}
		else if (keyJustPressed(inputState, SDLK_RETURN))
		{
			debugHandleConsoleInput(console);
		}

		if (inputState->textEntered[0])
		{
			int32 inputTextLength = strlen(inputState->textEntered);
			append(&console->input, inputState->textEntered, inputTextLength);
		}
		renderDebugData(debugState, uiState, uiBuffer);
		renderDebugConsole(debugState, &debugState->console, uiState, uiBuffer);
	}
}