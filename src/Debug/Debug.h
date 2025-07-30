/*
 * Copyright (c) 2016-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Gfx/Colour.h>
#include <SDL2/SDL_timer.h>
#include <Util/DeprecatedLinkedList.h>
#include <Util/EnumMap.h>
#include <Util/HashTable.h>
#include <Util/Pool.h>

#define GLUE_(a, b) a##b
#define GLUE(a, b) GLUE_(a, b)

#if BUILD_DEBUG

#    define DEBUG_BLOCK_T(name, tag)                                                                                    \
        static DebugCodeData* GLUE(debugBlockData____, __LINE__) = debugFindOrAddCodeData(makeString(name, true), tag); \
        DebugBlock GLUE(debugBlock____, __LINE__)(GLUE(debugBlockData____, __LINE__))
#    define DEBUG_FUNCTION_T(tag) DEBUG_BLOCK_T(__FUNCTION__, tag)

#    define DEBUG_BLOCK(name) DEBUG_BLOCK_T(name, DebugCodeDataTag::Misc)
#    define DEBUG_FUNCTION() DEBUG_FUNCTION_T(DebugCodeDataTag::Misc)

#    define DEBUG_ARENA(arena, name)                                               \
        static String GLUE(debugArenaName____, __LINE__) = makeString(name, true); \
        debugTrackArena(globalDebugState, arena, GLUE(debugArenaName____, __LINE__))

#    define DEBUG_POOL(pool, name)                                                \
        static String GLUE(debugPoolName____, __LINE__) = makeString(name, true); \
        debugTrackPool(globalDebugState, pool, GLUE(debugPoolName____, __LINE__))

#    define DEBUG_ASSETS() debugTrackAssets(globalDebugState)
#    define DEBUG_BEGIN_RENDER_BUFFER(bufferName, profileName) debugStartTrackingRenderBuffer(globalDebugState, bufferName, profileName)
#    define DEBUG_DRAW_CALL(shader, texture, itemCount) debugTrackDrawCall(globalDebugState, shader, texture, itemCount)
#    define DEBUG_TRACK_RENDER_BUFFER_CHUNK() debugTrackRenderBufferChunk(globalDebugState)
#    define DEBUG_END_RENDER_BUFFER() debugEndTrackingRenderBuffer(globalDebugState)

#else

#    define DEBUG_BLOCK(...)
#    define DEBUG_FUNCTION(...)
#    define DEBUG_BLOCK_T(...)
#    define DEBUG_FUNCTION_T(...)

#    define DEBUG_ARENA(...)
#    define DEBUG_POOL(...)
#    define DEBUG_ASSETS(...)

#    define DEBUG_BEGIN_RENDER_BUFFER(...)
#    define DEBUG_DRAW_CALL(...)
#    define DEBUG_TRACK_RENDER_BUFFER_CHUNK(...)
#    define DEBUG_END_RENDER_BUFFER(...)
#endif

inline struct DebugState* globalDebugState = nullptr;

#define DEBUG_FRAMES_COUNT 120
#define DEBUG_TOP_CODE_BLOCKS_COUNT 32

struct DebugArenaData : DeprecatedLinkedListNode<DebugArenaData> {
    String name;

    u32 blockCount[DEBUG_FRAMES_COUNT];
    smm totalSize[DEBUG_FRAMES_COUNT];
    smm usedSize[DEBUG_FRAMES_COUNT]; // How do we count free space in old blocks?
};

struct DebugPoolData : DeprecatedLinkedListNode<DebugPoolData> {
    String name;

    smm pooledItemCount[DEBUG_FRAMES_COUNT];
    smm totalItemCount[DEBUG_FRAMES_COUNT];
};

enum class DebugCodeDataTag : u8 {
    Misc,
    Highlight,
    Debug,
    Renderer,
    GameUpdate,
    Input,
    Simulation,
    UI,

    COUNT
};
EnumMap<DebugCodeDataTag, Colour> const debugCodeDataTagColors {
    Colour::from_rgb_255(255, 255, 255, 255), // White
    Colour::from_rgb_255(255, 0, 255, 255),   // Magenta
    Colour::from_rgb_255(128, 128, 128, 255), // Grey
    Colour::from_rgb_255(64, 255, 64, 255),   // Green
    Colour::from_rgb_255(32, 64, 255, 255),   // Blue
    Colour::from_rgb_255(255, 0, 0, 255),     // Red
    Colour::from_rgb_255(255, 64, 64, 255),   // Pink
    Colour::from_rgb_255(255, 128, 0, 255),   // Orange
};

struct DebugCodeData {
    String name;
    DebugCodeDataTag tag;

    u32 workingCallCount;
    u64 workingTotalCycleCount;
    u64 averageTotalCycleCount;

    u32 callCount[DEBUG_FRAMES_COUNT];
    u64 totalCycleCount[DEBUG_FRAMES_COUNT];
};

struct DebugCodeDataWrapper : DeprecatedLinkedListNode<DebugCodeDataWrapper> {
    DebugCodeData* data;
};

struct DebugDrawCallData {
    String shaderName;
    String textureName;
    u32 itemsDrawn;
};

#define DEBUG_DRAW_CALLS_RECORDED_PER_FRAME 8192
struct DebugRenderBufferData : DeprecatedLinkedListNode<DebugRenderBufferData> {
    String name;
    String renderProfileName;

    u32 drawCallCount[DEBUG_FRAMES_COUNT];
    DebugDrawCallData drawCalls[DEBUG_FRAMES_COUNT][DEBUG_DRAW_CALLS_RECORDED_PER_FRAME];
    u32 chunkCount[DEBUG_FRAMES_COUNT];

    u64 startTime[DEBUG_FRAMES_COUNT];
    u64 endTime[DEBUG_FRAMES_COUNT];
};

struct DebugAssetData // Not a linked list because there's only one asset system!
{
    // AssetsArena
    u32 arenaBlockCount[DEBUG_FRAMES_COUNT];
    smm arenaTotalSize[DEBUG_FRAMES_COUNT];
    smm arenaUsedSize[DEBUG_FRAMES_COUNT];

    // Asset stats
    s32 assetCount[DEBUG_FRAMES_COUNT];
    s32 loadedAssetCount[DEBUG_FRAMES_COUNT];

    // Asset memory
    smm assetMemoryAllocated[DEBUG_FRAMES_COUNT];
    smm maxAssetMemoryAllocated[DEBUG_FRAMES_COUNT];
    smm assetsByNameSize[DEBUG_FRAMES_COUNT];
};

struct DebugState {
    MemoryArena arena;
    bool showDebugData;
    bool captureDebugData;

    u32 readingFrameIndex;
    u32 writingFrameIndex;
    u64 frameStartCycle[DEBUG_FRAMES_COUNT];
    u64 frameEndCycle[DEBUG_FRAMES_COUNT];

    DebugArenaData arenaDataSentinel;
    DebugPoolData poolDataSentinel;
    DebugRenderBufferData renderBufferDataSentinel;
    DebugRenderBufferData* currentRenderBuffer;
    DebugAssetData assetData; // Not a sentinel because there's only one asset system!

    HashTable<DebugCodeData> codeData;

    // Processed stuff
    DebugCodeDataWrapper topCodeBlocksFreeListSentinel;
    DebugCodeDataWrapper topCodeBlocksSentinel;
};

void debugInit();
void updateAndRenderDebugData(DebugState* debugState);

void debugTrackArena(DebugState* debugState, MemoryArena* arena, String name);

template<typename T>
T* findOrCreateDebugData(DebugState* debugState, String name, T* sentinel)
{
    T* result = nullptr;

    T* data = sentinel->nextNode;
    while (data != sentinel) {
        if (equals(data->name, name)) {
            result = data;
            break;
        }
        data = data->nextNode;
    }

    if (result == nullptr) {
        result = debugState->arena.allocate<T>();
        addToLinkedList(result, sentinel);
        result->name = name;
    }

    return result;
}

template<typename T>
void debugTrackPool(DebugState* debugState, Pool<T>* pool, String name)
{
    DebugPoolData* poolData = findOrCreateDebugData(debugState, name, &debugState->poolDataSentinel);
    u32 frameIndex = debugState->writingFrameIndex;

    if (pool) {
        poolData->pooledItemCount[frameIndex] = pool->available_item_count();
        poolData->totalItemCount[frameIndex] = pool->total_created_item_count();
    }
}

void debugTrackAssets(DebugState* debugState);
void debugStartTrackingRenderBuffer(DebugState* debugState, String renderBufferName, String renderProfileName);
void debugTrackDrawCall(DebugState* debugState, String shaderName, String textureName, u32 itemsDrawn);
void debugTrackRenderBufferChunk(DebugState* debugState);
void debugEndTrackingRenderBuffer(DebugState* debugState);
void debugTrackProfile(String name, u64 cycleCount, DebugCodeDataTag tag = DebugCodeDataTag::Misc);

inline DebugCodeData* debugFindOrAddCodeData(String name, DebugCodeDataTag tag)
{
    DebugCodeData* result = globalDebugState->codeData.findOrAdd(name);
    result->name = name;
    result->tag = tag;

    return result;
}

struct DebugBlock {
    DebugCodeData* codeData;
    u64 startTime;

    explicit DebugBlock(DebugCodeData* codeData)
    {
        this->codeData = codeData;
        this->startTime = SDL_GetPerformanceCounter();
    }

    ~DebugBlock()
    {
        u64 cycleCount = SDL_GetPerformanceCounter() - this->startTime;
        codeData->workingCallCount++;
        codeData->workingTotalCycleCount += cycleCount;
    }
};
