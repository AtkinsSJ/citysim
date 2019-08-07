#pragma once

void debugInit()
{
	bootstrapArena(DebugState, globalDebugState, debugArena);
	globalDebugState->showDebugData = false;
	globalDebugState->captureDebugData = true;
	globalDebugState->readingFrameIndex = DEBUG_FRAMES_COUNT - 1;

	initLinkedListSentinel(&globalDebugState->arenaDataSentinel);
	initLinkedListSentinel(&globalDebugState->renderBufferDataSentinel);

	initHashTable(&globalDebugState->codeData, 0.5f, 2048);

	initLinkedListSentinel(&globalDebugState->topCodeBlocksFreeListSentinel);
	initLinkedListSentinel(&globalDebugState->topCodeBlocksSentinel);
	for (u32 i=0; i<DEBUG_TOP_CODE_BLOCKS_COUNT; i++)
	{
		DebugCodeDataWrapper *item = allocateStruct<DebugCodeDataWrapper>(&globalDebugState->debugArena);
		addToLinkedList(item, &globalDebugState->topCodeBlocksFreeListSentinel);
	}
}

void processDebugData(DebugState *debugState)
{
	DEBUG_FUNCTION_T(DCDT_Debug);
	
	u32 oldWritingFrameIndex = debugState->writingFrameIndex;
	debugState->frameEndCycle[debugState->writingFrameIndex] = SDL_GetPerformanceCounter();
	if (debugState->captureDebugData)
	{
		debugState->readingFrameIndex = debugState->writingFrameIndex;
		debugState->writingFrameIndex = (debugState->writingFrameIndex + 1) % DEBUG_FRAMES_COUNT;
	}
	debugState->frameStartCycle[debugState->writingFrameIndex] = SDL_GetPerformanceCounter();

	moveAllNodes(&debugState->topCodeBlocksSentinel, &debugState->topCodeBlocksFreeListSentinel);
	ASSERT(linkedListIsEmpty(&debugState->topCodeBlocksSentinel)); //List we just freed is not empty!

	for (auto it = iterate(&debugState->codeData); !it.isDone; next(&it))
	{
		DebugCodeData *code = get(it);

		// Move the `working` stuff into the correct frame
		if (debugState->captureDebugData)
		{
			code->callCount[oldWritingFrameIndex] = code->workingCallCount;
			code->totalCycleCount[oldWritingFrameIndex] = code->workingTotalCycleCount;
			code->averageTotalCycleCount = 0;
			for (s32 frameIndex = 0; frameIndex < DEBUG_FRAMES_COUNT; frameIndex++)
			{
				code->averageTotalCycleCount += code->totalCycleCount[frameIndex];
			}
			code->averageTotalCycleCount /= DEBUG_FRAMES_COUNT;
		}
		code->workingCallCount = 0;
		code->workingTotalCycleCount = 0;


		//
		// Calculate new top blocks list
		//

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

		ASSERT(countNodes(&debugState->topCodeBlocksSentinel) + countNodes(&debugState->topCodeBlocksFreeListSentinel) == DEBUG_TOP_CODE_BLOCKS_COUNT); //We lost a top code blocks node!
	}
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

	RenderBuffer *renderBuffer;
	Camera *camera;
	s8 textShaderID;
	s8 untexturedShaderID;

	u32 charsLastPrinted;
};

void initDebugTextState(DebugTextState *textState, BitmapFont *font, V4 textColor, f32 screenEdgePadding, bool upwards, bool alignLeft)
{
	*textState = {};

	textState->renderBuffer = &renderer->debugBuffer;
	textState->camera = &renderer->uiCamera;

	textState->progressUpwards = upwards;
	if (alignLeft)
	{
		textState->hAlign = ALIGN_LEFT;
		textState->pos.x = screenEdgePadding;
	}
	else
	{
		textState->hAlign = ALIGN_RIGHT;
		textState->pos.x = renderer->uiCamera.size.x - screenEdgePadding;
	}

	if (upwards) 
	{
		textState->pos.y = renderer->uiCamera.size.y - screenEdgePadding;
	}
	else
	{
		textState->pos.y = screenEdgePadding;
	}
	textState->font = font;
	textState->color = textColor;
	textState->maxWidth = renderer->uiCamera.size.x - (2*screenEdgePadding);

	textState->textShaderID = renderer->shaderIds.text;
	textState->untexturedShaderID = renderer->shaderIds.untextured;
}

void debugTextOut(DebugTextState *textState, String text, bool doHighlight = false, V4 *color = null)
{
	s32 align = textState->hAlign;
	if (textState->progressUpwards) align |= ALIGN_BOTTOM;
	else                            align |= ALIGN_TOP;

	textState->charsLastPrinted = text.length;
	V4 textColor = (color != null) ? *color : textState->color;

	V2 textSize = calculateTextSize(textState->font, text, textState->maxWidth);
	V2 topLeft  = calculateTextPosition(textState->pos, textSize, align);

	Rect2 bounds = rectPosSize(topLeft, textSize);

	if (doHighlight && contains(bounds, textState->camera->mousePos))
	{
		drawSingleRect(textState->renderBuffer, bounds, textState->untexturedShaderID, textColor * 0.5f);
		drawText(textState->renderBuffer, textState->font, text, bounds, align, color255(0, 0, 0, 255), textState->textShaderID);
	}
	else
	{
		drawText(textState->renderBuffer, textState->font, text, bounds, align, textColor, textState->textShaderID);
	}

	if (textState->progressUpwards)
	{
		textState->pos.y -= bounds.h;
	}
	else
	{
		textState->pos.y += bounds.h;
	}
}

void renderDebugData(DebugState *debugState)
{
	DEBUG_FUNCTION_T(DCDT_Debug);
	BitmapFont *font = getFont(makeString("debug"));
	RenderBuffer *renderBuffer = &renderer->debugBuffer;

	u64 cyclesPerSecond = SDL_GetPerformanceFrequency();
	u32 rfi = debugState->readingFrameIndex;
	drawSingleRect(renderBuffer, rectXYWH(0,0,renderer->uiCamera.size.x, renderer->uiCamera.size.y), renderer->shaderIds.untextured, color255(0,0,0,128));
	
	// Draw a "nice" chart!
	{
		f32 graphHeight = 150.0f;
		drawSingleRect(renderBuffer, rectXYWH(0, renderer->uiCamera.size.y - graphHeight, renderer->uiCamera.size.x, 1), renderer->shaderIds.untextured, color255(255, 255, 255, 128));
		drawSingleRect(renderBuffer, rectXYWH(0, renderer->uiCamera.size.y - graphHeight*2, renderer->uiCamera.size.x, 1), renderer->shaderIds.untextured, color255(255, 255, 255, 128));
		f32 targetCyclesPerFrame = cyclesPerSecond / 60.0f;
		f32 barWidth = renderer->uiCamera.size.x / (f32)DEBUG_FRAMES_COUNT;
		f32 barHeightPerCycle = graphHeight / targetCyclesPerFrame;
		V4 barColor = color255(255, 0, 0, 128);
		V4 activeBarColor = color255(255, 255, 0, 128);
		u32 barIndex = 0;
		DrawRectsGroup *rectsGroup = beginRectsGroupUntextured(renderBuffer, renderer->shaderIds.untextured, DEBUG_FRAMES_COUNT);
		for (u32 fi = debugState->writingFrameIndex + 1;
				 fi != debugState->writingFrameIndex;
				 fi = wrap<u32>(fi + 1, DEBUG_FRAMES_COUNT))
		{
			u64 frameCycles = debugState->frameEndCycle[fi] - debugState->frameStartCycle[fi];
			f32 barHeight = barHeightPerCycle * (f32)frameCycles;
			addUntexturedRect(rectsGroup, rectXYWH(barWidth * barIndex++, renderer->uiCamera.size.y - barHeight, barWidth, barHeight), fi == rfi ? activeBarColor : barColor);
		}
		endRectsGroup(rectsGroup);
	}

	DebugTextState textState;
	initDebugTextState(&textState, font, makeWhite(), 16.0f, false, true);

	u32 framesAgo = wrap<u32>(debugState->writingFrameIndex - rfi, DEBUG_FRAMES_COUNT);
	debugTextOut(&textState, myprintf("Examining {0} frames ago", {formatInt(framesAgo)}));

	// Asset system
	{
		DebugAssetData *assetData = &debugState->assetData;
		smm totalAssetMemory = assetData->assetMemoryAllocated[rfi] + assetData->assetsByNameSize[rfi] + assetData->arenaTotalSize[rfi];
		smm usedAssetMemory = assetData->assetMemoryAllocated[rfi] + assetData->assetsByNameSize[rfi] + assetData->arenaUsedSize[rfi];
		debugTextOut(&textState, myprintf("Asset system: {0}/{1} assets loaded, using {2} bytes ({3} allocated)\n    {4} bytes in arena, {5} bytes in assets, {6} bytes in hashtables", {
			formatInt(assetData->loadedAssetCount[rfi]),
			formatInt(assetData->assetCount[rfi]),
			formatInt(usedAssetMemory),
			formatInt(totalAssetMemory),

			formatInt(assetData->arenaTotalSize[rfi]),
			formatInt(assetData->assetMemoryAllocated[rfi]),
			formatInt(assetData->assetsByNameSize[rfi])
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
		DebugRenderBufferData *renderBufferData = debugState->renderBufferDataSentinel.nextNode;
		while (renderBufferData != &debugState->renderBufferDataSentinel)
		{
			s32 drawCallCount = renderBufferData->drawCallCount[rfi];
			s32 itemsDrawn = 0;
			for (s32 i=0; i<drawCallCount; i++)
			{
				itemsDrawn += renderBufferData->drawCalls[rfi][i].itemsDrawn;
			}
			debugTextOut(&textState, myprintf("Render buffer '{0}': {1} items drawn, in {2} batches. ({3} chunks)", {
				renderBufferData->name,
				formatInt(itemsDrawn),
				formatInt(drawCallCount),
				formatInt(renderBufferData->chunkCount[rfi])
			}));
			renderBufferData = renderBufferData->nextNode;
		}
	}

	debugTextOut(&textState, myprintf("There are {0} cycles in a second", {formatInt(cyclesPerSecond)}));

	// Top code blocks
	{
		debugTextOut(&textState, myprintf("{0}| {1}| {2}| {3}| {4}", {
			formatString("Code", 40),
			formatString("Total cycles", 26, false),
			formatString("Calls", 10, false),
			formatString("Avg Cycles", 26, false),
			formatString("2-second average cycles", 26, false)
		}));

		debugTextOut(&textState, repeatChar('-', textState.charsLastPrinted));
		DebugCodeDataWrapper *topBlock = debugState->topCodeBlocksSentinel.nextNode;
		f32 msPerCycle = 1000.0f / (f32)cyclesPerSecond;
		while (topBlock != &debugState->topCodeBlocksSentinel)
		{
			DebugCodeData *code = topBlock->data;
			f32 totalCycles = (f32)code->totalCycleCount[rfi];
			f32 averageCycles = totalCycles / (f32)code->callCount[rfi];
			debugTextOut(&textState, myprintf("{0}| {1} ({2}ms)| {3}| {4} ({5}ms)| {6} ({7}ms)", {
				formatString(code->name, 40),
				formatString(formatInt(code->totalCycleCount[rfi]), 16, false),
				formatString(formatFloat((f32)totalCycles * msPerCycle, 2), 5, false),
				formatString(formatInt(code->callCount[rfi]), 10, false),
				formatString(formatInt((s32)averageCycles), 16, false),
				formatString(formatFloat(averageCycles * msPerCycle, 2), 5, false),
				formatString(formatInt(code->averageTotalCycleCount), 10, false),
				formatString(formatFloat(code->averageTotalCycleCount * msPerCycle, 2), 5, false),
			}), true, debugCodeDataTagColors + code->tag);
			topBlock = topBlock->nextNode;
		}
	}

	// Put FPS in top right
	initDebugTextState(&textState, font, makeWhite(), 16.0f, false, false);
	{
		String smsForFrame = makeString("???");
		String sfps = makeString("???");
		if (rfi != debugState->writingFrameIndex)
		{
			f32 msForFrame = (f32) (debugState->frameEndCycle[rfi] - debugState->frameStartCycle[rfi]) / (f32)(cyclesPerSecond/1000);
			smsForFrame = formatFloat(msForFrame, 2);
			sfps = formatFloat(1000.0f / max(msForFrame, 1.0f), 2);
		}
		debugTextOut(&textState, myprintf("FPS: {0} ({1}ms)", {sfps, smsForFrame}));
	}
}

void updateAndRenderDebugData(DebugState *debugState, InputState *inputState)
{
	DEBUG_FUNCTION_T(DCDT_Debug);
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
		debugState->readingFrameIndex = wrap<u32>(debugState->readingFrameIndex - 1, DEBUG_FRAMES_COUNT);
	}
	else if (keyJustPressed(inputState, SDLK_PAGEUP))
	{
		debugState->readingFrameIndex = wrap<u32>(debugState->readingFrameIndex + 1, DEBUG_FRAMES_COUNT);
	}

	if (keyJustPressed(inputState, SDLK_INSERT))
	{
		// Output draw call data
		logDebug("****************** DRAW CALLS ******************");
		DebugRenderBufferData *renderBufferData = debugState->renderBufferDataSentinel.nextNode;
		u32 rfi = debugState->readingFrameIndex;
		while (renderBufferData != &debugState->renderBufferDataSentinel)
		{
			s32 drawCallCount = renderBufferData->drawCallCount[rfi];
			logDebug("Buffer {0} ({1} calls)\n-------------------------------", {renderBufferData->name, formatInt(drawCallCount)});
			for (s32 i=0; i<drawCallCount; i++)
			{
				DebugDrawCallData *drawCall = renderBufferData->drawCalls[rfi] + i;
				logDebug("{0}: {1} item(s), shader '{2}', texture '{3}'", {formatInt(i), formatInt(drawCall->itemsDrawn), drawCall->shaderName, drawCall->textureName});
			}
			renderBufferData = renderBufferData->nextNode;
		}
	}

	processDebugData(debugState);

	if (debugState->showDebugData)
	{
		renderDebugData(debugState);
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
		result = allocateStruct<T>(&debugState->debugArena);
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

void debugTrackAssets(DebugState *debugState)
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

	// assetsByType HashTables
	for (s32 assetType = 0; assetType < AssetTypeCount; assetType++)
	{
		auto assetsByNameForType = assets->assetsByType[assetType];
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

void debugStartTrackingRenderBuffer(DebugState *debugState, String renderBufferName, String renderProfileName)
{
	u32 frameIndex = debugState->writingFrameIndex;

	if (debugState->currentRenderBuffer != null)
	{
		debugEndTrackingRenderBuffer(debugState);
	}

	DebugRenderBufferData *renderBufferData = findOrCreateDebugData(debugState, renderBufferName, &debugState->renderBufferDataSentinel);

	debugState->currentRenderBuffer = renderBufferData;
	renderBufferData->renderProfileName = renderProfileName;
	renderBufferData->drawCallCount[frameIndex] = 0;
	renderBufferData->chunkCount[frameIndex] = 1; // Start at 1 because we're already inside a chunk when this is called
	renderBufferData->startTime[frameIndex] = SDL_GetPerformanceCounter();
}

void debugEndTrackingRenderBuffer(DebugState *debugState)
{
	u32 frameIndex = debugState->writingFrameIndex;

	debugState->currentRenderBuffer->endTime[frameIndex] = SDL_GetPerformanceCounter();
	debugTrackProfile(debugState->currentRenderBuffer->renderProfileName, debugState->currentRenderBuffer->endTime[frameIndex] - debugState->currentRenderBuffer->startTime[frameIndex], DCDT_Renderer);

	debugState->currentRenderBuffer = null;
}

void debugTrackDrawCall(DebugState *debugState, String shaderName, String textureName, u32 itemsDrawn)
{
	DebugRenderBufferData *renderBufferData = debugState->currentRenderBuffer;

	u32 frameIndex = debugState->writingFrameIndex;

	u32 drawCallIndex = renderBufferData->drawCallCount[frameIndex]++;
	if (drawCallIndex < DEBUG_DRAW_CALLS_RECORDED_PER_FRAME)
	{
		DebugDrawCallData *drawCall = &renderBufferData->drawCalls[frameIndex][drawCallIndex];
		drawCall->shaderName = shaderName;
		drawCall->textureName = textureName;
		drawCall->itemsDrawn = itemsDrawn;
	}
}

void debugTrackRenderBufferChunk(DebugState *debugState)
{
	DebugRenderBufferData *renderBufferData = debugState->currentRenderBuffer;

	u32 frameIndex = debugState->writingFrameIndex;

	renderBufferData->chunkCount[frameIndex]++;
}

void debugTrackProfile(String name, u64 cycleCount, DebugCodeDataTag tag)
{
	DebugCodeData *data = debugFindOrAddCodeData(name, tag);
	data->workingCallCount++;
	data->workingTotalCycleCount += cycleCount;
}
