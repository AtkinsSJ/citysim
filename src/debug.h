#pragma once

#ifdef BUILD_DEBUG

#define DEBUG_FUNCTION()

#define DEBUG_ARENA(arena, name) debugTrackArena(globalDebugState, arena, name)

#else

#define DEBUG_FUNCTION(...) 
#define DEBUG_ARENA(...)

#endif

struct DebugArenaData
{
	char *name;

	uint32 blockCount;
	umm totalSize;
	umm usedSize; // How do we count free space in old blocks?

	DebugArenaData *next;
};

struct DebugState
{
	MemoryArena debugArena;
	bool showDebugData;

	DebugArenaData *firstArenaData;
};

void processDebugData(DebugState *debugState)
{
	if (debugState)
	{
		
	}
}

void renderDebugData(DebugState *debugState)
{
	if (debugState)
	{
		DebugArenaData *arena = debugState->firstArenaData;
		while (arena)
		{
			SDL_LogDebug(SDL_LOG_CATEGORY_SYSTEM, "Memory arena %s: %d blocks, %Iu used / %Iu allocated",
				         arena->name, arena->blockCount, arena->usedSize, arena->totalSize);
			arena = arena->next;
		}
	}
}

void debugTrackArena(DebugState *debugState, MemoryArena *arena, char *arenaName)
{
	if (debugState)
	{
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

		arenaData->blockCount = 0;
		arenaData->totalSize = 0;
		arenaData->usedSize = 0;

		if (arena->currentBlock)
		{
			arenaData->blockCount = 1;
			arenaData->totalSize = arena->currentBlock->size;
			arenaData->usedSize = arena->currentBlock->used;

			MemoryBlock *block = arena->currentBlock->prevBlock;
			while (block)
			{
				arenaData->blockCount++;
				arenaData->totalSize += block->size;
				arenaData->usedSize += block->size;

				block = block->prevBlock;
			}
		}
	}
}

static DebugState *globalDebugState;