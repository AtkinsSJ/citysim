#pragma once

#define DEBUG_BREAK() SDL_TriggerBreakpoint()

// SDL already does work to avoid MSVC warnings, but we still get them!
// So, I've added what SHOULD be a no-op wrapper around it, using the (a,b) comma trick.
// (Well, it's not a no-op in debug builds, but the release optimiser does make it a no-op!)
// - Sam, 10/07/2019
#define ASSERT(expr) if ((expr),true) {SDL_assert(expr);} else {}
#define ASSERT_PARANOID(expr) if ((expr),true) {SDL_assert_paranoid(expr);} else {}
#define ASSERT_RELEASE(expr) if ((expr),true) {SDL_assert_release(expr);} else {}

#if BUILD_DEBUG

	#define DEBUG_BLOCK_T(name, tag) \
			static DebugCodeData *GLUE(debugBlockData____, __LINE__) = debugFindOrAddCodeData(makeString(name, true), tag); \
			DebugBlock GLUE(debugBlock____, __LINE__) (GLUE(debugBlockData____, __LINE__))
	#define DEBUG_FUNCTION_T(tag) DEBUG_BLOCK_T(__FUNCTION__, tag)

	#define DEBUG_BLOCK(name) DEBUG_BLOCK_T(name, DCDT_Misc)
	#define DEBUG_FUNCTION() DEBUG_FUNCTION_T(DCDT_Misc)

	#define DEBUG_ARENA(arena, name) \
			static String GLUE(debugArenaName____, __LINE__) = makeString(name, true); \
			debugTrackArena(globalDebugState, arena, GLUE(debugArenaName____, __LINE__))

	#define DEBUG_ASSETS(assets) debugTrackAssets(globalDebugState, assets)
	#define DEBUG_DRAW_CALL(shader, texture, itemCount) debugTrackDrawCall(globalDebugState, shader, texture, itemCount)
	#define DEBUG_BEGIN_RENDER_BUFFER(bufferName) debugStartTrackingRenderBuffer(globalDebugState, bufferName)

#else

	#define DEBUG_BLOCK(...)
	#define DEBUG_FUNCTION(...)
	#define DEBUG_BLOCK_T(...)
	#define DEBUG_FUNCTION_T(...)

	#define DEBUG_ARENA(...)
	#define DEBUG_ASSETS(...)
	#define DEBUG_DRAW_CALL(...)
	#define DEBUG_BEGIN_RENDER_BUFFER(...)
#endif

struct DebugState *globalDebugState = 0;

#define DEBUG_FRAMES_COUNT 120
#define DEBUG_TOP_CODE_BLOCKS_COUNT 32

struct DebugArenaData : LinkedListNode<DebugArenaData>
{
	String name;

	u32 blockCount[DEBUG_FRAMES_COUNT];
	smm totalSize[DEBUG_FRAMES_COUNT];
	smm usedSize[DEBUG_FRAMES_COUNT]; // How do we count free space in old blocks?
};

enum DebugCodeDataTag
{
	DCDT_Misc,
	DCDT_Debugging,
	DCDT_Renderer,
	DCDT_GameUpdate,
	DCDT_Input,

	DebugCodeDataTagCount
};
V4 debugCodeDataTagColors[DebugCodeDataTagCount] = {
	color255(255, 255, 255, 255),
	color255(255,   0, 255, 255),
	color255( 64, 255,  64, 255),
	color255( 64, 128, 255, 255),
	color255(255,  64,  64, 255),
};

struct DebugCodeData
{
	String name;
	DebugCodeDataTag tag;

	u32 workingCallCount;
	u64 workingTotalCycleCount;
	u64 averageTotalCycleCount;

	u32 callCount[DEBUG_FRAMES_COUNT];
	u64 totalCycleCount[DEBUG_FRAMES_COUNT];
};

struct DebugCodeDataWrapper : LinkedListNode<DebugCodeDataWrapper>
{
	DebugCodeData *data;
};

struct DebugDrawCallData
{
	String shaderName;
	String textureName;
	u32 itemsDrawn;
};

#define DEBUG_DRAW_CALLS_RECORDED_PER_FRAME 8192
struct DebugRenderBufferData : LinkedListNode<DebugRenderBufferData>
{
	String name;

	u64 timeToSort[DEBUG_FRAMES_COUNT];
	u32 drawCallCount[DEBUG_FRAMES_COUNT];
	DebugDrawCallData drawCalls[DEBUG_FRAMES_COUNT][DEBUG_DRAW_CALLS_RECORDED_PER_FRAME];
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

struct DebugState
{
	MemoryArena debugArena;
	bool showDebugData;
	bool captureDebugData;

	u32 readingFrameIndex;
	u32 writingFrameIndex;
	u64 frameStartCycle[DEBUG_FRAMES_COUNT];
	u64 frameEndCycle[DEBUG_FRAMES_COUNT];

	DebugArenaData arenaDataSentinel;
	DebugRenderBufferData renderBufferDataSentinel;
	DebugRenderBufferData *currentRenderBuffer;
	DebugAssetData assetData; // Not a sentinel because there's only one asset system!

	HashTable<DebugCodeData> codeData;

	// Processed stuff
	DebugCodeDataWrapper topCodeBlocksFreeListSentinel;
	DebugCodeDataWrapper topCodeBlocksSentinel;
};

void debugInit();
void updateAndRenderDebugData(DebugState *debugState, struct InputState *inputState, struct Renderer *renderer);

void debugTrackArena(DebugState *debugState, MemoryArena *arena, String arenaName);
void debugTrackAssets(DebugState *debugState, struct AssetManager *assets);
void debugTrackDrawCall(DebugState *debugState, String shaderName, String textureName, u32 itemsDrawn);
void debugStartTrackingRenderBuffer(DebugState *debugState, struct String renderBufferName);
DebugCodeData *debugFindOrAddCodeData(String name, DebugCodeDataTag tag)
{
	DebugCodeData *result = findOrAdd(&globalDebugState->codeData, name);
	result->name = name;
	result->tag = tag;

	return result;
}

struct DebugBlock
{
	DebugCodeData *codeData;
	u64 startTime;

	DebugBlock(DebugCodeData *codeData)
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
