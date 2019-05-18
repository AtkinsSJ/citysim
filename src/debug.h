#pragma once

#if BUILD_DEBUG
	#if defined(_MSC_VER)
		// Pauses the MS debugger
		#define DEBUG_BREAK() __debugbreak()
	#else
		#define DEBUG_BREAK() {*(int *)0 = 0;}
	#endif

	void ASSERT(bool expr, char *format, std::initializer_list<String> args)
	{
		if(!(expr))
		{
			logError(format, args);
			DEBUG_BREAK();
		}
	}

	#define DEBUG_BLOCK(name) DebugBlock GLUE(debugBlock____, __COUNTER__) (makeString(name))
	#define DEBUG_FUNCTION() DEBUG_BLOCK(__FUNCTION__)

	#define DEBUG_ARENA(arena, name) debugTrackArena(globalDebugState, arena, makeString(name))
	#define DEBUG_ASSETS(assets) debugTrackAssets(globalDebugState, assets)
	#define DEBUG_DRAW_CALL(shader, texture, itemCount) debugTrackDrawCall(globalDebugState, shader, texture, itemCount)
	#define DEBUG_BEGIN_RENDER_BUFFER(buffer) debugStartTrackingRenderBuffer(globalDebugState, buffer)
#else
	#define DEBUG_BREAK()

	// We put a dummy thing here to stop the compiler complaining
	// "local variable is initialized but not referenced"
	// if a variable is only used in expr.
	#define ASSERT(expr, ...) if (expr) {};

	#define DEBUG_BLOCK(...) 
	#define DEBUG_FUNCTION(...) 

	#define DEBUG_ARENA(...)
	#define DEBUG_ASSETS(...)
	#define DEBUG_DRAW_CALL(...)
	#define DEBUG_BEGIN_RENDER_BUFFER(...)
#endif

struct DebugState *globalDebugState = 0;

#define DEBUG_FRAMES_COUNT 120
#define DEBUG_TOP_CODE_BLOCKS_COUNT 20

struct DebugArenaData : LinkedListNode<DebugArenaData>
{
	String name;

	u32 blockCount[DEBUG_FRAMES_COUNT];
	smm totalSize[DEBUG_FRAMES_COUNT];
	smm usedSize[DEBUG_FRAMES_COUNT]; // How do we count free space in old blocks?
};

struct DebugCodeData : LinkedListNode<DebugCodeData>
{
	String name;

	u32 callCount[DEBUG_FRAMES_COUNT];
	u64 totalCycleCount[DEBUG_FRAMES_COUNT];
	u64 averageCycleCount[DEBUG_FRAMES_COUNT];
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

#define DEBUG_DRAW_CALLS_RECORDED_PER_FRAME 2048
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
	DebugCodeData codeDataSentinel;
	DebugRenderBufferData renderBufferDataSentinel;
	DebugRenderBufferData *currentRenderBuffer;
	DebugAssetData assetData; // Not a sentinel because there's only one asset system!

	// Processed stuff
	DebugCodeDataWrapper topCodeBlocksFreeListSentinel;
	DebugCodeDataWrapper topCodeBlocksSentinel;
};

void debugTrackArena(DebugState *debugState, MemoryArena *arena, String arenaName);
void debugTrackAssets(DebugState *debugState, struct AssetManager *assets);
void debugTrackCodeCall(DebugState *debugState, String name, u64 cycleCount);
void debugTrackDrawCall(DebugState *debugState, String shaderName, String textureName, u32 itemsDrawn);
void debugStartTrackingRenderBuffer(DebugState *debugState, struct RenderBuffer *renderBuffer);

struct DebugBlock
{
	String name;
	u64 startTime;

	DebugBlock(String name)
	{
		this->name = name;
		this->startTime = SDL_GetPerformanceCounter();
	}

	~DebugBlock()
	{
		u64 cycleCount = SDL_GetPerformanceCounter() - this->startTime;
		debugTrackCodeCall(globalDebugState, this->name, cycleCount);
	}
};