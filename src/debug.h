#pragma once

#ifdef BUILD_DEBUG

#define GLUE_(a, b) a ## b
#define GLUE(a, b) GLUE_(a, b)
#define DEBUG_BLOCK(name) DebugBlock GLUE(debugBlock____, __COUNTER__) (stringFromChars(name))
#define DEBUG_FUNCTION() DEBUG_BLOCK(__FUNCTION__)

#define DEBUG_ARENA(arena, name) debugTrackArena(globalDebugState, arena, stringFromChars(name))

#else

#define DEBUG_BLOCK(...) 
#define DEBUG_FUNCTION(...) 
#define DEBUG_ARENA(...)

#endif

static struct DebugState *globalDebugState = 0;

#define DEBUG_FRAMES_COUNT 120
#define DEBUG_TOP_CODE_BLOCKS_COUNT 20

struct DebugArenaData
{
	String name;

	u32 blockCount[DEBUG_FRAMES_COUNT];
	umm totalSize[DEBUG_FRAMES_COUNT];
	umm usedSize[DEBUG_FRAMES_COUNT]; // How do we count free space in old blocks?

	DLinkedListMembers(DebugArenaData);
};

struct DebugCodeData
{
	String name;

	u32 callCount[DEBUG_FRAMES_COUNT];
	u64 totalCycleCount[DEBUG_FRAMES_COUNT];
	u64 averageCycleCount[DEBUG_FRAMES_COUNT];

	DLinkedListMembers(DebugCodeData);
};

struct DebugCodeDataWrapper
{
	DebugCodeData *data;
	DLinkedListMembers(DebugCodeDataWrapper);
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

	// Processed stuff
	DebugCodeDataWrapper topCodeBlocksFreeListSentinel;
	DebugCodeDataWrapper topCodeBlocksSentinel;

	struct BitmapFont *font;
};

void debugTrackArena(DebugState *debugState, MemoryArena *arena, String arenaName)
{
	if (debugState)
	{
		u32 frameIndex = debugState->writingFrameIndex;

		DebugArenaData *arenaData = debugState->arenaDataSentinel.next;
		bool found = false;
		while (arenaData != &debugState->arenaDataSentinel)
		{
			if (equals(arenaData->name, arenaName))
			{
				found = true;
				break;
			}
			
			arenaData = arenaData->next;
		}

		if (!found)
		{
			arenaData = PushStruct(&debugState->debugArena, DebugArenaData);
			DLinkedListInsertBefore(arenaData, &debugState->arenaDataSentinel);
			arenaData->name = arenaName;
		}

		arenaData->blockCount[frameIndex] = 0;
		arenaData->totalSize[frameIndex] = 0;
		arenaData->usedSize[frameIndex] = 0;

		if (arena) // So passing null just keeps it zeroed out
		{
			if (arena->currentBlock)
			{
				arenaData->blockCount[frameIndex] = 1;
				arenaData->totalSize[frameIndex] = arena->currentBlock->size;
				arenaData->usedSize[frameIndex] = arena->currentBlock->used;

				MemoryBlock *block = arena->currentBlock->prevBlock;
				while (block)
				{
					arenaData->blockCount[frameIndex]++;
					arenaData->totalSize[frameIndex] += block->size;
					arenaData->usedSize[frameIndex] += block->size;

					block = block->prevBlock;
				}
			}
		}
	}
}

void debugTrackCodeCall(DebugState *debugState, String name, u64 cycleCount)
{
	if (debugState)
	{
		u32 frameIndex = debugState->writingFrameIndex;

		DebugCodeData *codeData = debugState->codeDataSentinel.next;
		bool found = false;
		while (codeData != &debugState->codeDataSentinel)
		{
			if (equals(codeData->name, name))
			{
				found = true;
				break;
			}
			
			codeData = codeData->next;
		}

		if (!found)
		{
			codeData = PushStruct(&debugState->debugArena, DebugCodeData);
			DLinkedListInsertBefore(codeData, &debugState->codeDataSentinel);

			codeData->name = name;
		}

		codeData->callCount[frameIndex]++;
		codeData->totalCycleCount[frameIndex] += cycleCount;
		codeData->averageCycleCount[frameIndex] = codeData->totalCycleCount[frameIndex] / codeData->callCount[frameIndex];
	}
}

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