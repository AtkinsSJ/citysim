#pragma once

#ifdef BUILD_DEBUG

#define GLUE_(a, b) a ## b
#define GLUE(a, b) GLUE_(a, b)
#define DEBUG_BLOCK(name) DebugBlock GLUE(debugBlock____, __COUNTER__) (stringFromChars(name))
#define DEBUG_FUNCTION() DEBUG_BLOCK(__FUNCTION__)

#define DEBUG_ARENA(arena, name) debugTrackArena(globalDebugState, arena, stringFromChars(name))
#define DEBUG_RENDER_BUFFER(buffer, drawCallCount) debugTrackRenderBuffer(globalDebugState, buffer, drawCallCount)

#else

#define DEBUG_BLOCK(...) 
#define DEBUG_FUNCTION(...) 
#define DEBUG_ARENA(...)
#define DEBUG_RENDER_BUFFER(...)

#endif


static struct DebugState *globalDebugState = 0;

#define DEBUG_FRAMES_COUNT 120
#define DEBUG_TOP_CODE_BLOCKS_COUNT 20

struct DebugArenaData
{
	String name;

	uint32 blockCount[DEBUG_FRAMES_COUNT];
	umm totalSize[DEBUG_FRAMES_COUNT];
	umm usedSize[DEBUG_FRAMES_COUNT]; // How do we count free space in old blocks?

	DLinkedListMembers(DebugArenaData);
};

struct DebugCodeData
{
	String name;

	uint32 callCount[DEBUG_FRAMES_COUNT];
	uint64 totalCycleCount[DEBUG_FRAMES_COUNT];
	uint64 averageCycleCount[DEBUG_FRAMES_COUNT];

	DLinkedListMembers(DebugCodeData);
};

struct DebugCodeDataWrapper
{
	DebugCodeData *data;
	DLinkedListMembers(DebugCodeDataWrapper);
};

struct DebugRenderBufferData
{
	String name;

	uint32 itemCount[DEBUG_FRAMES_COUNT];
	uint32 drawCallCount[DEBUG_FRAMES_COUNT];

	DLinkedListMembers(DebugRenderBufferData);
};

struct DebugState
{
	MemoryArena debugArena;
	bool showDebugData;
	bool captureDebugData;

	uint32 readingFrameIndex;
	uint32 writingFrameIndex;
	uint64 frameStartCycle[DEBUG_FRAMES_COUNT];
	uint64 frameEndCycle[DEBUG_FRAMES_COUNT];

	DebugArenaData arenaDataSentinel;
	DebugCodeData codeDataSentinel;
	DebugRenderBufferData renderBufferDataSentinel;

	// Processed stuff
	DebugCodeDataWrapper topCodeBlocksFreeListSentinel;
	DebugCodeDataWrapper topCodeBlocksSentinel;

	struct BitmapFont *font;
};

void debugTrackArena(DebugState *debugState, MemoryArena *arena, String arenaName);
void debugTrackRenderBuffer(DebugState *debugState, struct RenderBuffer *renderBuffer, uint32 drawCallCount);
void debugTrackCodeCall(DebugState *debugState, String name, uint64 cycleCount);

struct DebugBlock
{
	String name;
	uint64 startTime;

	DebugBlock(String name)
	{
		this->name = name;
		this->startTime = SDL_GetPerformanceCounter();
	}

	~DebugBlock()
	{
		uint64 cycleCount = SDL_GetPerformanceCounter() - this->startTime;
		debugTrackCodeCall(globalDebugState, this->name, cycleCount);
	}
};