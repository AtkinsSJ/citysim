#pragma once

#ifdef BUILD_DEBUG

#define DEBUG_FUNCTION()

#define DEBUG_ARENA(arena, name) debugTrackArena(globalDebugState, arena, name)

#else

#define DEBUG_FUNCTION(...) 
#define DEBUG_ARENA(...)

#endif

#define DEBUG_FRAMES_COUNT 120

struct DebugArenaData
{
	char *name;

	uint32 blockCount[DEBUG_FRAMES_COUNT];
	umm totalSize[DEBUG_FRAMES_COUNT];
	umm usedSize[DEBUG_FRAMES_COUNT]; // How do we count free space in old blocks?

	DebugArenaData *next;
};

struct DebugState
{
	MemoryArena debugArena;
	bool showDebugData;

	uint32 readingFrameIndex;
	uint32 writingFrameIndex;
	DebugArenaData *firstArenaData;
};

void processDebugData(DebugState *debugState)
{
	if (debugState)
	{



		debugState->readingFrameIndex = debugState->writingFrameIndex;
		debugState->writingFrameIndex = (debugState->writingFrameIndex + 1) % DEBUG_FRAMES_COUNT;
	}
}

void renderDebugData(DebugState *debugState)
{
	if (debugState)
	{
		uint32 frameIndex = debugState->readingFrameIndex;
		DebugArenaData *arena = debugState->firstArenaData;
		while (arena)
		{
			// NB: %Iu is Microsoft's non-standard version of %zu, which doesn't work right now
			SDL_LogDebug(SDL_LOG_CATEGORY_SYSTEM, "Memory arena %s: %d blocks, %Iu used / %Iu allocated",
				         arena->name, arena->blockCount[frameIndex], arena->usedSize[frameIndex], arena->totalSize[frameIndex]);
			arena = arena->next;
		}
	}
}

void debugTrackArena(DebugState *debugState, MemoryArena *arena, char *arenaName)
{
	if (debugState)
	{
		uint32 frameIndex = debugState->writingFrameIndex;

		// find the arena
		DebugArenaData *arenaData = debugState->firstArenaData;
		bool found = false;
		while (arenaData)
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
			arenaData->next = debugState->firstArenaData;
			debugState->firstArenaData = arenaData;

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

static DebugState *globalDebugState;