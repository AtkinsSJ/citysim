#pragma once

void debugInit()
{
	bootstrapArena(DebugState, globalDebugState, debugArena);
	globalDebugState->showDebugData = false;
	globalDebugState->captureDebugData = true;
	globalDebugState->readingFrameIndex = DEBUG_FRAMES_COUNT - 1;

	initLinkedListSentinel(&globalDebugState->arenaDataSentinel);
	initLinkedListSentinel(&globalDebugState->codeDataSentinel);
	initLinkedListSentinel(&globalDebugState->renderBufferDataSentinel);

	initLinkedListSentinel(&globalDebugState->topCodeBlocksFreeListSentinel);
	initLinkedListSentinel(&globalDebugState->topCodeBlocksSentinel);
	for (u32 i=0; i<DEBUG_TOP_CODE_BLOCKS_COUNT; i++)
	{
		DebugCodeDataWrapper *item = PushStruct(&globalDebugState->debugArena, DebugCodeDataWrapper);
		addToLinkedList(item, &globalDebugState->topCodeBlocksFreeListSentinel);
	}
}

void clearDebugFrame(DebugState *debugState, s32 frameIndex)
{
	DebugCodeData *codeData = debugState->codeDataSentinel.nextNode;
	while (codeData != &debugState->codeDataSentinel)
	{
		codeData->callCount[frameIndex] = 0;
		codeData->totalCycleCount[frameIndex] = 0;
		codeData->averageCycleCount[frameIndex] = 0;
		codeData = codeData->nextNode;
	}
}

void processDebugData(DebugState *debugState)
{
	DEBUG_FUNCTION();
	
	debugState->frameEndCycle[debugState->writingFrameIndex] = SDL_GetPerformanceCounter();
	if (debugState->captureDebugData)
	{
		debugState->readingFrameIndex = debugState->writingFrameIndex;
		debugState->writingFrameIndex = (debugState->writingFrameIndex + 1) % DEBUG_FRAMES_COUNT;
	}
	debugState->frameStartCycle[debugState->writingFrameIndex] = SDL_GetPerformanceCounter();

	moveAllNodes(&debugState->topCodeBlocksSentinel, &debugState->topCodeBlocksFreeListSentinel);
	ASSERT(linkedListIsEmpty(&debugState->topCodeBlocksSentinel), "List we just freed is not empty!");

	// Calculate new top blocks list
	DebugCodeData *code = debugState->codeDataSentinel.nextNode;
	while (code != &debugState->codeDataSentinel)
	{
		// Find spot on list. If we fail, target is the sentinel so it all still works!
		DebugCodeDataWrapper *target = debugState->topCodeBlocksSentinel.nextNode;
		bool foundSmallerItem = false;
		while (target != &debugState->topCodeBlocksSentinel)
		{
			if (target->data->totalCycleCount[debugState->readingFrameIndex]
			    < code->totalCycleCount[debugState->readingFrameIndex])
			{
				foundSmallerItem = true;
				break;
			}
			target = target->nextNode;
		}

		bool freeListHasItem = !linkedListIsEmpty(&debugState->topCodeBlocksFreeListSentinel);

		DebugCodeDataWrapper *item = null;
		if (freeListHasItem)
		{
			item = debugState->topCodeBlocksFreeListSentinel.nextNode;
		}
		else if (foundSmallerItem)
		{
			item = debugState->topCodeBlocksSentinel.prevNode;
		}

		if (item)
		{
			// NB: If the item we want was at the end of the list, it could ALSO be the target!
			// So, we only move it around if that's not the case. Otherwise it gets added to a
			// new list that's just itself, and it disappears off into the aether forever.
			// - Sam, 13/05/2019
			if (item != target)
			{
				removeFromLinkedList(item);
				addToLinkedList(item, target);
			}

			item->data = code;
		}

		ASSERT(countNodes(&debugState->topCodeBlocksSentinel) + countNodes(&debugState->topCodeBlocksFreeListSentinel) == DEBUG_TOP_CODE_BLOCKS_COUNT, "We lost a top code blocks node!");

		code = code->nextNode;
	}

	// Zero-out new writing frame.
	clearDebugFrame(debugState, debugState->writingFrameIndex);
}

struct DebugTextState
{
	V2 pos;
	Alignment hAlign;
	char buffer[1024];
	BitmapFont *font;
	V4 color;
	f32 maxWidth;
	bool progressUpwards;

	UIState *uiState;

	u32 charsLastPrinted;
};

void initDebugTextState(DebugTextState *textState, UIState *uiState, BitmapFont *font, V4 textColor, V2 screenSize, f32 screenEdgePadding, bool upwards, bool alignLeft)
{
	*textState = {};

	textState->progressUpwards = upwards;
	if (alignLeft)
	{
		textState->hAlign = ALIGN_LEFT;
		textState->pos.x = screenEdgePadding;
	}
	else
	{
		textState->hAlign = ALIGN_RIGHT;
		textState->pos.x = screenSize.x - screenEdgePadding;
	}

	if (upwards) 
	{
		textState->pos.y = screenSize.y - screenEdgePadding;
	}
	else
	{
		textState->pos.y = screenEdgePadding;
	}
	textState->font = font;
	textState->color = textColor;
	textState->maxWidth = screenSize.x - (2*screenEdgePadding);

	textState->uiState = uiState;
}

void debugTextOut(DebugTextState *textState, String text)
{
	s32 align = textState->hAlign;
	if (textState->progressUpwards) align |= ALIGN_BOTTOM;
	else                            align |= ALIGN_TOP;

	textState->charsLastPrinted = text.length;
	Rect2 resultRect = uiText(textState->uiState, textState->font, text, textState->pos,
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

void renderDebugData(DebugState *debugState, UIState *uiState)
{
	BitmapFont *font = getFont(globalAppState.assets, makeString("debug"));

	RenderBuffer *uiBuffer = uiState->uiBuffer;

	u64 cyclesPerSecond = SDL_GetPerformanceFrequency();
	u32 rfi = debugState->readingFrameIndex;
	drawRect(uiBuffer, rectXYWH(0,0,uiBuffer->camera.size.x, uiBuffer->camera.size.y),
		     100, color255(0,0,0,128));

	DebugTextState textState;
	initDebugTextState(&textState, uiState, font, makeWhite(), uiBuffer->camera.size, 16.0f, false, true);

	u32 framesAgo = WRAP(debugState->writingFrameIndex - rfi, DEBUG_FRAMES_COUNT);
	debugTextOut(&textState, myprintf("Examining {0} frames ago", {formatInt(framesAgo)}));

	// Asset system
	{
		DebugAssetData *assets = &debugState->assetData;
		smm totalAssetMemory = assets->assetMemoryAllocated[rfi] + assets->assetsByNameSize[rfi] + assets->arenaTotalSize[rfi];
		smm usedAssetMemory = assets->assetMemoryAllocated[rfi] + assets->assetsByNameSize[rfi] + assets->arenaUsedSize[rfi];
		debugTextOut(&textState, myprintf("Asset system: {0}/{1} assets loaded, using {2} bytes ({3} allocated)", {
			formatInt(assets->loadedAssetCount[rfi]),
			formatInt(assets->assetCount[rfi]),
			formatInt(usedAssetMemory),
			formatInt(totalAssetMemory)
		}));
	}

	// Memory arenas
	{
		DebugArenaData *arena = debugState->arenaDataSentinel.nextNode;
		while (arena != &debugState->arenaDataSentinel)
		{
			debugTextOut(&textState, myprintf("Memory arena {0}: {1} blocks, {2} used / {3} allocated ({4}%)", {
				arena->name,
				formatInt(arena->blockCount[rfi]),
				formatInt(arena->usedSize[rfi]),
				formatInt(arena->totalSize[rfi]),
				formatFloat(100.0f * (f32)arena->usedSize[rfi] / (f32)arena->totalSize[rfi], 1)
			}));
			arena = arena->nextNode;
		}
	}

	// Render buffers
	{
		DebugRenderBufferData *renderBuffer = debugState->renderBufferDataSentinel.nextNode;
		while (renderBuffer != &debugState->renderBufferDataSentinel)
		{
			s32 drawCallCount = renderBuffer->drawCallCount[rfi];
			s32 itemsDrawn = 0;
			for (s32 i=0; i<drawCallCount; i++)
			{
				itemsDrawn += renderBuffer->drawCalls[rfi][i].itemsDrawn;
			}
			debugTextOut(&textState, myprintf("Render buffer '{0}': {1} items drawn, in {2} batches", {
				renderBuffer->name,
				formatInt(itemsDrawn),
				formatInt(drawCallCount)
			}));
			renderBuffer = renderBuffer->nextNode;
		}
	}

	debugTextOut(&textState, myprintf("There are {0} cycles in a second", {formatInt(cyclesPerSecond)}));

	// Top code blocks
	{
		debugTextOut(&textState, myprintf("{0}| {1}| {2}| {3}", {
			formatString("Code", 30), formatString("Total cycles", 20, false),
			formatString("Calls", 20, false), formatString("Avg Cycles", 20, false)
		}));

		debugTextOut(&textState, repeatChar('-', textState.charsLastPrinted));
		DebugCodeDataWrapper *topBlock = debugState->topCodeBlocksSentinel.nextNode;
		while (topBlock != &debugState->topCodeBlocksSentinel)
		{
			DebugCodeData *code = topBlock->data;
			debugTextOut(&textState, myprintf("{0}| {1}| {2}| {3}", {
				formatString(code->name, 30),
				formatString(formatInt(code->totalCycleCount[rfi]), 20, false),
				formatString(formatInt(code->callCount[rfi]), 20, false),
				formatString(formatInt(code->averageCycleCount[rfi]), 20, false)
			}));
			topBlock = topBlock->nextNode;
		}
	}

	// Draw a "nice" chart!
	{
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

	// Put FPS in top right
	initDebugTextState(&textState, uiState, font, makeWhite(), uiBuffer->camera.size, 16.0f, false, false);
	{
		String smsForFrame = makeString("???");
		String sfps = makeString("???");
		if (rfi != debugState->writingFrameIndex)
		{
			f32 msForFrame = (f32) (debugState->frameEndCycle[rfi] - debugState->frameStartCycle[rfi]) / (f32)(cyclesPerSecond/1000);
			smsForFrame = formatFloat(msForFrame, 2);
			sfps = formatFloat(1000.0f / MAX(msForFrame, 1), 2);
		}
		debugTextOut(&textState, myprintf("FPS: {0} ({1}ms)", {sfps, smsForFrame}));
	}
}

void debugUpdate(DebugState *debugState, InputState *inputState, UIState *uiState)
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

	if (keyJustPressed(inputState, SDLK_INSERT))
	{
		// Output draw call data
		logDebug("****************** DRAW CALLS ******************");
		DebugRenderBufferData *renderBuffer = debugState->renderBufferDataSentinel.nextNode;
		u32 rfi = debugState->readingFrameIndex;
		while (renderBuffer != &debugState->renderBufferDataSentinel)
		{
			s32 drawCallCount = renderBuffer->drawCallCount[rfi];
			logDebug("Buffer {0} ({1} calls)\n-------------------------------", {renderBuffer->name, formatInt(drawCallCount)});
			for (s32 i=0; i<drawCallCount; i++)
			{
				DebugDrawCallData *drawCall = renderBuffer->drawCalls[rfi] + i;
				logDebug("{0}: {1} item(s), shader '{2}', texture '{3}'", {formatInt(i), formatInt(drawCall->itemsDrawn), drawCall->shaderName, drawCall->textureName});
			}
			renderBuffer = renderBuffer->nextNode;
		}
	}

	processDebugData(debugState);

	if (debugState->showDebugData)
	{
		renderDebugData(debugState, uiState);
	}
}

template<typename T>
T *findOrCreateDebugData(DebugState *debugState, String name, T *sentinel)
{
	T *result = null;

	T *data = sentinel->nextNode;
	while (data != sentinel)
	{
		if (equals(data->name, name))
		{
			result = data;
			break;
		}
		data = data->nextNode;
	}

	if (result == null)
	{
		result = PushStruct(&debugState->debugArena, T);
		addToLinkedList(result, sentinel);
		result->name = name;
	}

	return result;
}

void debugTrackArena(DebugState *debugState, MemoryArena *arena, String name)
{
	DebugArenaData *arenaData = findOrCreateDebugData(debugState, name, &debugState->arenaDataSentinel);

	u32 frameIndex = debugState->writingFrameIndex;

	arenaData->blockCount[frameIndex] = 0;
	arenaData->totalSize[frameIndex] = 0;
	arenaData->usedSize[frameIndex] = 0;

	if (arena) // So passing null just keeps it zeroed out
	{
		if (arena->currentBlock)
		{
			arenaData->blockCount[frameIndex] = 1;
			arenaData->totalSize [frameIndex] = arena->currentBlock->size;
			arenaData->usedSize  [frameIndex] = arena->currentBlock->used;

			MemoryBlock *block = arena->currentBlock->prevBlock;
			while (block)
			{
				arenaData->blockCount[frameIndex]++;
				arenaData->totalSize[frameIndex] += block->size;
				arenaData->usedSize[frameIndex]  += block->size;

				block = block->prevBlock;
			}
		}
	}
}

void debugTrackAssets(DebugState *debugState, AssetManager *assets)
{
	DebugAssetData *assetData = &debugState->assetData;
	u32 frameIndex = debugState->writingFrameIndex;

	assetData->arenaBlockCount        [frameIndex] = 0;
	assetData->arenaTotalSize         [frameIndex] = 0;
	assetData->arenaUsedSize          [frameIndex] = 0;
	assetData->assetCount             [frameIndex] = 0;
	assetData->loadedAssetCount       [frameIndex] = 0;
	assetData->assetMemoryAllocated   [frameIndex] = 0;
	assetData->maxAssetMemoryAllocated[frameIndex] = 0;
	assetData->assetsByNameSize       [frameIndex] = 0;

	// The assets arena
	MemoryArena *arena = &assets->assetArena;
	if (arena->currentBlock)
	{
		assetData->arenaBlockCount[frameIndex] = 1;
		assetData->arenaTotalSize [frameIndex] = arena->currentBlock->size;
		assetData->arenaUsedSize  [frameIndex] = arena->currentBlock->used;

		MemoryBlock *block = arena->currentBlock->prevBlock;
		while (block)
		{
			assetData->arenaBlockCount[frameIndex]++;
			assetData->arenaTotalSize [frameIndex] += block->size;
			assetData->arenaUsedSize  [frameIndex] += block->size;

			block = block->prevBlock;
		}
	}

	// assetsByName HashTables
	for (s32 assetType = 0; assetType < AssetTypeCount; assetType++)
	{
		auto assetsByNameForType = assets->assetsByName[assetType];
		assetData->assetsByNameSize[frameIndex] += assetsByNameForType.capacity * sizeof(assetsByNameForType.entries[0]);
	}

	// The asset-memory stuff
	for (auto it = iterate(&assets->allAssets); !it.isDone; next(&it))
	{
		Asset *asset = get(it);

		if (asset->state == AssetState_Loaded) assetData->loadedAssetCount[frameIndex]++;
	}
	assetData->assetMemoryAllocated   [frameIndex] = assets->assetMemoryAllocated;
	assetData->maxAssetMemoryAllocated[frameIndex] = assets->maxAssetMemoryAllocated;

	assetData->assetCount[frameIndex] = (s32)assets->allAssets.count;
}

void debugTrackCodeCall(DebugState *debugState, String name, u64 cycleCount)
{
	DebugCodeData *codeData = findOrCreateDebugData(debugState, name, &debugState->codeDataSentinel);

	u32 frameIndex = debugState->writingFrameIndex;

	codeData->callCount[frameIndex]++;
	codeData->totalCycleCount[frameIndex] += cycleCount;
	codeData->averageCycleCount[frameIndex] = codeData->totalCycleCount[frameIndex] / codeData->callCount[frameIndex];
}

void debugTrackDrawCall(DebugState *debugState, String shaderName, String textureName, u32 itemsDrawn)
{
	DebugRenderBufferData *renderBufferData = debugState->currentRenderBuffer;

	u32 frameIndex = debugState->writingFrameIndex;

	DebugDrawCallData *drawCall = &renderBufferData->drawCalls[frameIndex][renderBufferData->drawCallCount[frameIndex]++];
	drawCall->shaderName = shaderName;
	drawCall->textureName = textureName;
	drawCall->itemsDrawn = itemsDrawn;
}

void debugStartTrackingRenderBuffer(DebugState *debugState, RenderBuffer *renderBuffer)
{
	DebugRenderBufferData *renderBufferData = findOrCreateDebugData(debugState, renderBuffer->name, &debugState->renderBufferDataSentinel);
	u32 frameIndex = debugState->writingFrameIndex;

	debugState->currentRenderBuffer = renderBufferData;
	renderBufferData->drawCallCount[frameIndex] = 0;
}
