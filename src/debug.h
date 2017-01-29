#pragma once

#ifdef BUILD_DEBUG

#define GLUE_(a, b) a ## b
#define GLUE(a, b) GLUE_(a, b)
#define DEBUG_BLOCK(name) DebugBlock GLUE(debugBlock____, __COUNTER__) (name)
#define DEBUG_FUNCTION() DEBUG_BLOCK(__FUNCTION__)

#define DEBUG_ARENA(arena, name) debugTrackArena(globalDebugState, arena, name)

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
	char *name;

	uint32 blockCount[DEBUG_FRAMES_COUNT];
	umm totalSize[DEBUG_FRAMES_COUNT];
	umm usedSize[DEBUG_FRAMES_COUNT]; // How do we count free space in old blocks?

	DLinkedListMembers(DebugArenaData);
};

struct DebugCodeData
{
	char *name;

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

struct DebugConsole
{
	StringBuffer input;
	int32 outputLineCount;
	StringBuffer *outputLines;
	int32 currentOutputLine;
};

struct DebugState
{
	MemoryArena debugArena;
	bool showDebugData;
	bool captureDebugData;

	DebugConsole console;

	uint32 readingFrameIndex;
	uint32 writingFrameIndex;
	uint64 frameStartCycle[DEBUG_FRAMES_COUNT];
	uint64 frameEndCycle[DEBUG_FRAMES_COUNT];

	DebugArenaData arenaDataSentinel;
	DebugCodeData codeDataSentinel;

	// Processed stuff
	DebugCodeDataWrapper topCodeBlocksFreeListSentinel;
	DebugCodeDataWrapper topCodeBlocksSentinel;

	struct BitmapFont *font;
};

void debugInit(BitmapFont *font)
{
	bootstrapArena(DebugState, globalDebugState, debugArena);
	globalDebugState->showDebugData = true;
	globalDebugState->captureDebugData = true;
	globalDebugState->readingFrameIndex = DEBUG_FRAMES_COUNT - 1;
	globalDebugState->font = font;

	DebugConsole *console = &globalDebugState->console;

	console->input = newStringBuffer(&globalDebugState->debugArena, 255);
	console->outputLineCount = 64;
	console->outputLines = PushArray(&globalDebugState->debugArena, StringBuffer, console->outputLineCount);
	for (int32 i=0; i < console->outputLineCount; i++)
	{
		console->outputLines[i] = newStringBuffer(&globalDebugState->debugArena, 255);
	}

	DLinkedListInit(&globalDebugState->arenaDataSentinel);
	DLinkedListInit(&globalDebugState->codeDataSentinel);

	DLinkedListInit(&globalDebugState->topCodeBlocksFreeListSentinel);
	DLinkedListInit(&globalDebugState->topCodeBlocksSentinel);
	for (uint32 i=0; i<DEBUG_TOP_CODE_BLOCKS_COUNT; i++)
	{
		DebugCodeDataWrapper *item = PushStruct(&globalDebugState->debugArena, DebugCodeDataWrapper);
		DLinkedListInsertBefore(item, &globalDebugState->topCodeBlocksFreeListSentinel);
	}
}

void debugTrackArena(DebugState *debugState, MemoryArena *arena, char *arenaName)
{
	if (debugState)
	{
		uint32 frameIndex = debugState->writingFrameIndex;

		DebugArenaData *arenaData = debugState->arenaDataSentinel.next;
		bool found = false;
		while (arenaData != &debugState->arenaDataSentinel)
		{
			if (strcmp(arenaData->name, arenaName) == 0)
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

void debugTrackCodeCall(DebugState *debugState, char *name, uint64 cycleCount)
{
	if (debugState)
	{
		uint32 frameIndex = debugState->writingFrameIndex;

		DebugCodeData *codeData = debugState->codeDataSentinel.next;
		bool found = false;
		while (codeData != &debugState->codeDataSentinel)
		{
			if (strcmp(codeData->name, name) == 0)
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
	char *name;
	uint64 startTime;

	DebugBlock(char *name)
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