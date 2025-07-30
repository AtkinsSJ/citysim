/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Debug.h"
#include "../font.h"
#include "../input.h"
#include <Assets/AssetManager.h>
#include <Gfx/Renderer.h>

void debugInit()
{
    globalDebugState = MemoryArena::bootstrap<DebugState>("Debug"_s);
    globalDebugState->showDebugData = false;
    globalDebugState->captureDebugData = true;
    globalDebugState->readingFrameIndex = DEBUG_FRAMES_COUNT - 1;

    initLinkedListSentinel(&globalDebugState->arenaDataSentinel);
    initLinkedListSentinel(&globalDebugState->poolDataSentinel);
    initLinkedListSentinel(&globalDebugState->renderBufferDataSentinel);

    initHashTable(&globalDebugState->codeData, 0.5f, 2048);

    initLinkedListSentinel(&globalDebugState->topCodeBlocksFreeListSentinel);
    initLinkedListSentinel(&globalDebugState->topCodeBlocksSentinel);
    for (u32 i = 0; i < DEBUG_TOP_CODE_BLOCKS_COUNT; i++) {
        DebugCodeDataWrapper* item = globalDebugState->arena.allocate<DebugCodeDataWrapper>();
        addToLinkedList(item, &globalDebugState->topCodeBlocksFreeListSentinel);
    }
}

void processDebugData(DebugState* debugState)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::Debug);

    u32 oldWritingFrameIndex = debugState->writingFrameIndex;
    debugState->frameEndCycle[debugState->writingFrameIndex] = SDL_GetPerformanceCounter();
    if (debugState->captureDebugData) {
        debugState->readingFrameIndex = debugState->writingFrameIndex;
        debugState->writingFrameIndex = (debugState->writingFrameIndex + 1) % DEBUG_FRAMES_COUNT;
    }
    debugState->frameStartCycle[debugState->writingFrameIndex] = SDL_GetPerformanceCounter();

    moveAllNodes(&debugState->topCodeBlocksSentinel, &debugState->topCodeBlocksFreeListSentinel);
    ASSERT(linkedListIsEmpty(&debugState->topCodeBlocksSentinel)); // List we just freed is not empty!

    for (auto it = debugState->codeData.iterate(); it.hasNext(); it.next()) {
        DebugCodeData* code = it.get();

        // Move the `working` stuff into the correct frame
        if (debugState->captureDebugData) {
            code->callCount[oldWritingFrameIndex] = code->workingCallCount;
            code->totalCycleCount[oldWritingFrameIndex] = code->workingTotalCycleCount;
            code->averageTotalCycleCount = 0;
            for (s32 frameIndex = 0; frameIndex < DEBUG_FRAMES_COUNT; frameIndex++) {
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
        DebugCodeDataWrapper* target = debugState->topCodeBlocksSentinel.nextNode;
        bool foundSmallerItem = false;
        while (target != &debugState->topCodeBlocksSentinel) {
            if (target->data->totalCycleCount[debugState->readingFrameIndex]
                < code->totalCycleCount[debugState->readingFrameIndex]) {
                foundSmallerItem = true;
                break;
            }
            target = target->nextNode;
        }

        bool freeListHasItem = !linkedListIsEmpty(&debugState->topCodeBlocksFreeListSentinel);

        DebugCodeDataWrapper* item = nullptr;
        if (freeListHasItem) {
            item = debugState->topCodeBlocksFreeListSentinel.nextNode;
        } else if (foundSmallerItem) {
            item = debugState->topCodeBlocksSentinel.prevNode;
        }

        if (item) {
            // NB: If the item we want was at the end of the list, it could ALSO be the target!
            // So, we only move it around if that's not the case. Otherwise it gets added to a
            // new list that's just itself, and it disappears off into the aether forever.
            // - Sam, 13/05/2019
            if (item != target) {
                removeFromLinkedList(item);
                addToLinkedList(item, target);
            }

            item->data = code;
        }

        ASSERT(countNodes(&debugState->topCodeBlocksSentinel) + countNodes(&debugState->topCodeBlocksFreeListSentinel) == DEBUG_TOP_CODE_BLOCKS_COUNT); // We lost a top code blocks node!
    }
}

void clearNewFrameDebugData(DebugState* debugState)
{
    u32 frameIndex = debugState->writingFrameIndex;

    // Clear away old data before we record the new
    // This stops ghost data hanging around if the thing isn't tracked at all this frame

    // Memory arenas
    {
        DebugArenaData* arena = debugState->arenaDataSentinel.nextNode;
        while (arena != &debugState->arenaDataSentinel) {
            arena->blockCount[frameIndex] = 0;
            arena->totalSize[frameIndex] = 0;
            arena->usedSize[frameIndex] = 0;
            arena = arena->nextNode;
        }
    }

    // Pools
    {
        DebugPoolData* pool = debugState->poolDataSentinel.nextNode;
        while (pool != &debugState->poolDataSentinel) {
            pool->pooledItemCount[frameIndex] = 0;
            pool->totalItemCount[frameIndex] = 0;
            pool = pool->nextNode;
        }
    }

    // Render buffers
    {
        DebugRenderBufferData* renderBufferData = debugState->renderBufferDataSentinel.nextNode;
        while (renderBufferData != &debugState->renderBufferDataSentinel) {
            renderBufferData->drawCallCount[frameIndex] = 0;
            renderBufferData->chunkCount[frameIndex] = 0;
            renderBufferData->startTime[frameIndex] = 0;
            renderBufferData->endTime[frameIndex] = 0;

            renderBufferData = renderBufferData->nextNode;
        }
    }
}

struct DebugTextState {
    V2I pos;
    Alignment hAlign;
    char buffer[1024];
    BitmapFont* font;
    Colour color;
    s32 maxWidth;
    bool progressUpwards;

    RenderBuffer* renderBuffer;
    Camera* camera;
    s8 textShaderID;
    s8 untexturedShaderID;

    u32 charsLastPrinted;
};

void initDebugTextState(DebugTextState* textState, BitmapFont* font, Colour textColor, s32 screenEdgePadding, bool upwards, bool alignLeft)
{
    auto& renderer = the_renderer();
    *textState = {};

    textState->renderBuffer = &renderer.debug_buffer();
    textState->camera = &renderer.ui_camera();

    textState->progressUpwards = upwards;
    if (alignLeft) {
        textState->hAlign = ALIGN_LEFT;
        textState->pos.x = screenEdgePadding;
    } else {
        textState->hAlign = ALIGN_RIGHT;
        textState->pos.x = ceil_s32(textState->camera->size().x) - screenEdgePadding;
    }

    if (upwards) {
        textState->pos.y = ceil_s32(textState->camera->size().y) - screenEdgePadding;
    } else {
        textState->pos.y = screenEdgePadding;
    }
    textState->font = font;
    textState->color = textColor;
    textState->maxWidth = floor_s32(textState->camera->size().x) - (2 * screenEdgePadding);

    textState->textShaderID = renderer.shaderIds.text;
    textState->untexturedShaderID = renderer.shaderIds.untextured;
}

void debugTextOut(DebugTextState* textState, String text, bool doHighlight = false, Colour const* color = nullptr)
{
    s32 align = textState->hAlign;
    if (textState->progressUpwards)
        align |= ALIGN_BOTTOM;
    else
        align |= ALIGN_TOP;

    textState->charsLastPrinted = text.length;
    Colour textColor = (color != nullptr) ? *color : textState->color;

    V2I textSize = calculateTextSize(textState->font, text, textState->maxWidth);
    V2I topLeft = calculateTextPosition(textState->pos, textSize, align);

    Rect2I bounds = irectPosSize(topLeft, textSize);

    if (doHighlight && contains(bounds, textState->camera->mouse_position())) {
        drawSingleRect(textState->renderBuffer, bounds, textState->untexturedShaderID, textColor * 0.5f);
        drawText(textState->renderBuffer, textState->font, text, bounds, align, Colour::from_rgb_255(0, 0, 0, 255), textState->textShaderID);
    } else {
        drawText(textState->renderBuffer, textState->font, text, bounds, align, textColor, textState->textShaderID);
    }

    if (textState->progressUpwards) {
        textState->pos.y -= bounds.h;
    } else {
        textState->pos.y += bounds.h;
    }
}

void renderDebugData(DebugState* debugState)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::Debug);
    auto& renderer = the_renderer();

    // This is the only usage of getFont(String). Ideally we'd replace it with an
    // AssetRef, but there's no obvious place to put it. It can't go in the
    // DebugState because debugInit() is called before the Asset system is
    // initialised, so getAssetRef() won't work. (getAssetRef() interns the
    // string it's given, so it crashes if the Asset system isn't initialised.)
    // So for now, we're keeping this old method.
    // - Sam, 18/02/2020
    BitmapFont* font = getFont("debug.fnt"_s);
    RenderBuffer* renderBuffer = &renderer.debug_buffer();
    auto& ui_camera = renderer.ui_camera();

    u64 cyclesPerSecond = SDL_GetPerformanceFrequency();
    u32 rfi = debugState->readingFrameIndex;
    drawSingleRect(renderBuffer, rectXYWH(0, 0, ui_camera.size().x, ui_camera.size().y), renderer.shaderIds.untextured, Colour::from_rgb_255(0, 0, 0, 128));

    // Draw a "nice" chart!
    {
        float graphHeight = 150.0f;
        drawSingleRect(renderBuffer, rectXYWH(0, ui_camera.size().y - graphHeight, ui_camera.size().x, 1), renderer.shaderIds.untextured, Colour::from_rgb_255(255, 255, 255, 128));
        drawSingleRect(renderBuffer, rectXYWH(0, ui_camera.size().y - graphHeight * 2, ui_camera.size().x, 1), renderer.shaderIds.untextured, Colour::from_rgb_255(255, 255, 255, 128));
        float targetCyclesPerFrame = cyclesPerSecond / 60.0f;
        float barWidth = ui_camera.size().x / (float)DEBUG_FRAMES_COUNT;
        float barHeightPerCycle = graphHeight / targetCyclesPerFrame;
        Colour barColor = Colour::from_rgb_255(255, 0, 0, 128);
        Colour activeBarColor = Colour::from_rgb_255(255, 255, 0, 128);
        u32 barIndex = 0;
        DrawRectsGroup* rectsGroup = beginRectsGroupUntextured(renderBuffer, renderer.shaderIds.untextured, DEBUG_FRAMES_COUNT);
        for (u32 fi = debugState->writingFrameIndex + 1;
            fi != debugState->writingFrameIndex;
            fi = wrap<u32>(fi + 1, DEBUG_FRAMES_COUNT)) {
            u64 frameCycles = debugState->frameEndCycle[fi] - debugState->frameStartCycle[fi];
            float barHeight = barHeightPerCycle * (float)frameCycles;
            addUntexturedRect(rectsGroup, rectXYWH(barWidth * barIndex++, ui_camera.size().y - barHeight, barWidth, barHeight), fi == rfi ? activeBarColor : barColor);
        }
        endRectsGroup(rectsGroup);
    }

    DebugTextState textState;
    initDebugTextState(&textState, font, Colour::white(), 16, false, true);

    u32 framesAgo = wrap<u32>(debugState->writingFrameIndex - rfi, DEBUG_FRAMES_COUNT);
    debugTextOut(&textState, myprintf("Examining {0} frames ago"_s, { formatInt(framesAgo) }));

    // Asset system
    {
        DebugAssetData* assetData = &debugState->assetData;
        smm totalAssetMemory = assetData->assetMemoryAllocated[rfi] + assetData->assetsByNameSize[rfi] + assetData->arenaTotalSize[rfi];
        smm usedAssetMemory = assetData->assetMemoryAllocated[rfi] + assetData->assetsByNameSize[rfi] + assetData->arenaUsedSize[rfi];
        debugTextOut(&textState, myprintf("Asset system: {0}/{1} assets loaded, using {2} bytes ({3} allocated)\n    {4} bytes in arena, {5} bytes in assets, {6} bytes in hashtables\n    sizeof(Asset) = {7}"_s, { formatInt(assetData->loadedAssetCount[rfi]), formatInt(assetData->assetCount[rfi]), formatInt(usedAssetMemory), formatInt(totalAssetMemory),

                                                                                                                                                                                                                       formatInt(assetData->arenaUsedSize[rfi]), formatInt(assetData->assetMemoryAllocated[rfi]), formatInt(assetData->assetsByNameSize[rfi]), formatInt(sizeof(Asset)) }));
    }

    // Memory arenas
    {
        DebugArenaData* arena = debugState->arenaDataSentinel.nextNode;
        while (arena != &debugState->arenaDataSentinel) {
            debugTextOut(&textState, myprintf("Memory arena {0}: {1} blocks, {2} used / {3} allocated ({4}%)"_s, { arena->name, formatInt(arena->blockCount[rfi]), formatInt(arena->usedSize[rfi]), formatInt(arena->totalSize[rfi]), formatFloat(100.0f * (float)arena->usedSize[rfi] / (float)arena->totalSize[rfi], 1) }));
            arena = arena->nextNode;
        }
    }

    // Pools
    {
        DebugPoolData* pool = debugState->poolDataSentinel.nextNode;
        while (pool != &debugState->poolDataSentinel) {
            debugTextOut(&textState, myprintf("Pool {0}: {1} / {2}"_s, { pool->name, formatInt(pool->pooledItemCount[rfi]), formatInt(pool->totalItemCount[rfi]) }));
            pool = pool->nextNode;
        }
    }

    // Render buffers
    {
        DebugRenderBufferData* renderBufferData = debugState->renderBufferDataSentinel.nextNode;
        while (renderBufferData != &debugState->renderBufferDataSentinel) {
            s32 drawCallCount = renderBufferData->drawCallCount[rfi];
            // Skip empty renderbuffers
            if (drawCallCount != 0) {
                s32 itemsDrawn = 0;
                for (s32 i = 0; i < drawCallCount; i++) {
                    itemsDrawn += renderBufferData->drawCalls[rfi][i].itemsDrawn;
                }
                debugTextOut(&textState, myprintf("Render buffer '{0}': {1} items drawn, in {2} batches. ({3} chunks)"_s, { renderBufferData->name, formatInt(itemsDrawn), formatInt(drawCallCount), formatInt(renderBufferData->chunkCount[rfi]) }));
            }
            renderBufferData = renderBufferData->nextNode;
        }
    }

    debugTextOut(&textState, myprintf("There are {0} cycles in a second"_s, { formatInt(cyclesPerSecond) }));

    // Top code blocks
    {
        debugTextOut(&textState, myprintf("{0}| {1}| {2}| {3}| {4}"_s, { formatString("Code", 60), formatString("Total cycles", 20, false), formatString("Calls", 10, false), formatString("Avg Cycles", 20, false), formatString("2-second avg cycles", 20, false) }));

        debugTextOut(&textState, repeatChar('-', textState.charsLastPrinted));
        DebugCodeDataWrapper* topBlock = debugState->topCodeBlocksSentinel.nextNode;
        float msPerCycle = 1000.0f / (float)cyclesPerSecond;
        while (topBlock != &debugState->topCodeBlocksSentinel) {
            DebugCodeData* code = topBlock->data;
            float totalCycles = (float)code->totalCycleCount[rfi];
            float averageCycles = totalCycles / (float)code->callCount[rfi];
            debugTextOut(&textState,
                myprintf("{0}| {1} ({2}ms)| {3}| {4} ({5}ms)| {6} ({7}ms)"_s,
                    {
                        formatString(code->name, 60),
                        formatString(formatInt(code->totalCycleCount[rfi]), 10, false),
                        formatString(formatFloat((float)totalCycles * msPerCycle, 2), 5, false),
                        formatString(formatInt(code->callCount[rfi]), 10, false),
                        formatString(formatInt((s32)averageCycles), 10, false),
                        formatString(formatFloat(averageCycles * msPerCycle, 2), 5, false),
                        formatString(formatInt(code->averageTotalCycleCount), 10, false),
                        formatString(formatFloat(code->averageTotalCycleCount * msPerCycle, 2), 5, false),
                    }),
                true, &debugCodeDataTagColors[code->tag]);
            topBlock = topBlock->nextNode;
        }
    }

    // Put FPS in top right
    initDebugTextState(&textState, font, Colour::white(), 16, false, false);
    {
        String smsForFrame = "???"_s;
        String sfps = "???"_s;
        if (rfi != debugState->writingFrameIndex) {
            float msForFrame = (float)(debugState->frameEndCycle[rfi] - debugState->frameStartCycle[rfi]) / (float)(cyclesPerSecond / 1000);
            smsForFrame = formatFloat(msForFrame, 2);
            sfps = formatFloat(1000.0f / max(msForFrame, 1.0f), 2);
        }
        debugTextOut(&textState, myprintf("FPS: {0} ({1}ms)"_s, { sfps, smsForFrame }));
    }
}

void updateAndRenderDebugData(DebugState* debugState)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::Debug);
    if (keyJustPressed(SDLK_F2)) {
        debugState->showDebugData = !debugState->showDebugData;
    }

    if (keyJustPressed(SDLK_PAUSE)) {
        debugState->captureDebugData = !debugState->captureDebugData;
    }

    if (keyJustPressed(SDLK_PAGEDOWN)) {
        debugState->readingFrameIndex = wrap<u32>(debugState->readingFrameIndex - 1, DEBUG_FRAMES_COUNT);
    } else if (keyJustPressed(SDLK_PAGEUP)) {
        debugState->readingFrameIndex = wrap<u32>(debugState->readingFrameIndex + 1, DEBUG_FRAMES_COUNT);
    }

    if (keyJustPressed(SDLK_INSERT)) {
        // Output draw call data
        logDebug("****************** DRAW CALLS ******************"_s);
        DebugRenderBufferData* renderBufferData = debugState->renderBufferDataSentinel.nextNode;
        u32 rfi = debugState->readingFrameIndex;
        while (renderBufferData != &debugState->renderBufferDataSentinel) {
            s32 drawCallCount = renderBufferData->drawCallCount[rfi];
            logDebug("Buffer {0} ({1} calls)\n-------------------------------"_s, { renderBufferData->name, formatInt(drawCallCount) });
            for (s32 i = 0; i < drawCallCount; i++) {
                DebugDrawCallData* drawCall = renderBufferData->drawCalls[rfi] + i;
                logDebug("{0}: {1} item(s), shader '{2}', texture '{3}'"_s, { formatInt(i), formatInt(drawCall->itemsDrawn), drawCall->shaderName, drawCall->textureName });
            }
            renderBufferData = renderBufferData->nextNode;
        }
    }

    processDebugData(debugState);

    if (debugState->showDebugData) {
        renderDebugData(debugState);
    }

    clearNewFrameDebugData(debugState);
}

void debugTrackArena(DebugState* debug_state, MemoryArena* arena, String name)
{
    DebugArenaData* arena_data = findOrCreateDebugData(debug_state, name, &debug_state->arenaDataSentinel);

    u32 frame_index = debug_state->writingFrameIndex;

    if (arena) // So passing null just keeps it zeroed out
    {
        auto const statistics = arena->get_statistics();
        arena_data->blockCount[frame_index] = statistics.block_count;
        arena_data->totalSize[frame_index] = statistics.total_size;
        arena_data->usedSize[frame_index] = statistics.used_size;
    }
}

void debugTrackAssets(DebugState* debugState)
{
    DebugAssetData* assetData = &debugState->assetData;
    u32 frameIndex = debugState->writingFrameIndex;

    assetData->arenaBlockCount[frameIndex] = 0;
    assetData->arenaTotalSize[frameIndex] = 0;
    assetData->arenaUsedSize[frameIndex] = 0;
    assetData->assetCount[frameIndex] = 0;
    assetData->loadedAssetCount[frameIndex] = 0;
    assetData->assetMemoryAllocated[frameIndex] = 0;
    assetData->maxAssetMemoryAllocated[frameIndex] = 0;
    assetData->assetsByNameSize[frameIndex] = 0;

    // The assets arena
    auto const statistics = asset_manager().arena.get_statistics();
    assetData->arenaBlockCount[frameIndex] = statistics.block_count;
    assetData->arenaTotalSize[frameIndex] = statistics.total_size;
    assetData->arenaUsedSize[frameIndex] = statistics.used_size;

    // assetsByType HashTables
    for (auto asset_type : enum_values<AssetType>()) {
        auto const& assetsByNameForType = asset_manager().assetsByType[asset_type];
        assetData->assetsByNameSize[frameIndex] += assetsByNameForType.capacity * sizeof(assetsByNameForType.entries[0]);
    }

    // The asset-memory stuff
    for (auto it = asset_manager().allAssets.iterate(); it.hasNext(); it.next()) {
        Asset* asset = it.get();

        if (asset->state == Asset::State::Loaded)
            assetData->loadedAssetCount[frameIndex]++;
    }
    assetData->assetMemoryAllocated[frameIndex] = asset_manager().assetMemoryAllocated;
    assetData->maxAssetMemoryAllocated[frameIndex] = asset_manager().maxAssetMemoryAllocated;

    assetData->assetCount[frameIndex] = asset_manager().allAssets.count;
}

void debugStartTrackingRenderBuffer(DebugState* debugState, String renderBufferName, String renderProfileName)
{
    u32 frameIndex = debugState->writingFrameIndex;

    if (debugState->currentRenderBuffer != nullptr) {
        debugEndTrackingRenderBuffer(debugState);
    }

    DebugRenderBufferData* renderBufferData = findOrCreateDebugData(debugState, renderBufferName, &debugState->renderBufferDataSentinel);

    debugState->currentRenderBuffer = renderBufferData;
    renderBufferData->renderProfileName = renderProfileName;
    renderBufferData->drawCallCount[frameIndex] = 0;
    renderBufferData->chunkCount[frameIndex] = 1; // Start at 1 because we're already inside a chunk when this is called
    renderBufferData->startTime[frameIndex] = SDL_GetPerformanceCounter();
}

void debugEndTrackingRenderBuffer(DebugState* debugState)
{
    if (debugState->currentRenderBuffer != nullptr) {
        u32 frameIndex = debugState->writingFrameIndex;

        debugState->currentRenderBuffer->endTime[frameIndex] = SDL_GetPerformanceCounter();
        debugTrackProfile(debugState->currentRenderBuffer->renderProfileName, debugState->currentRenderBuffer->endTime[frameIndex] - debugState->currentRenderBuffer->startTime[frameIndex], DebugCodeDataTag::Renderer);

        debugState->currentRenderBuffer = nullptr;
    }
}

void debugTrackDrawCall(DebugState* debugState, String shaderName, String textureName, u32 itemsDrawn)
{
    DebugRenderBufferData* renderBufferData = debugState->currentRenderBuffer;

    u32 frameIndex = debugState->writingFrameIndex;

    u32 drawCallIndex = renderBufferData->drawCallCount[frameIndex]++;
    if (drawCallIndex < DEBUG_DRAW_CALLS_RECORDED_PER_FRAME) {
        DebugDrawCallData* drawCall = &renderBufferData->drawCalls[frameIndex][drawCallIndex];
        drawCall->shaderName = shaderName;
        drawCall->textureName = textureName;
        drawCall->itemsDrawn = itemsDrawn;
    }
}

void debugTrackRenderBufferChunk(DebugState* debugState)
{
    DebugRenderBufferData* renderBufferData = debugState->currentRenderBuffer;

    u32 frameIndex = debugState->writingFrameIndex;

    renderBufferData->chunkCount[frameIndex]++;
}

void debugTrackProfile(String name, u64 cycleCount, DebugCodeDataTag tag)
{
    DebugCodeData* data = debugFindOrAddCodeData(name, tag);
    data->workingCallCount++;
    data->workingTotalCycleCount += cycleCount;
}
