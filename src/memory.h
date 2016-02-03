#ifndef MEMORY_H
#define MEMORY_H

#include <stdlib.h> // For calloc
#include <string.h>

#define KB(x) ((x) * 1024)
#define MB(x) ((x) * 1024 * 1024)
#define GB(x) ((x) * 1024 * 1024 * 1024)

#define ArrayCount(a) (sizeof(a) / sizeof(a[0]))
#define ArrayCountS(a) ((int)(ArrayCount(a)))

struct MemoryArena
{
	size_t size;
	size_t used;
	uint8 *memory;
	bool hasTemporaryArenaOpen;
};

struct TemporaryMemoryArena
{
	MemoryArena arena;
	bool isOpen;
	MemoryArena *parentArena;
};

bool initMemoryArena(MemoryArena *arena, size_t size)
{
	bool succeeded = false;
	arena->memory = (uint8*)calloc(size, 1);
	if (arena->memory)
	{
		arena->size = size;
		arena->used = 0;
		arena->hasTemporaryArenaOpen = false;
		succeeded = true;
	}
	
	return succeeded;
}

void *allocate(MemoryArena *arena, size_t size)
{
	ASSERT((arena->used + size) <= arena->size, "Arena out of memory!");
	ASSERT(!arena->hasTemporaryArenaOpen, "Allocating to an arena while temporary subArena is open!");
	
	void *result = arena->memory + arena->used;
	memset(result, 0, size);
	
	arena->used += size;
	
	return result;
}

void *allocate(TemporaryMemoryArena *tempArena, size_t size)
{
	ASSERT(tempArena->isOpen, "TemporaryMemory already ended!");
	return allocate(&tempArena->arena, size);
}

void ResetMemoryArena(MemoryArena *arena)
{
	arena->used = 0;
}

MemoryArena allocateSubArena(MemoryArena *arena, size_t size)
{
	MemoryArena subArena = {};

	subArena.memory = (uint8 *)allocate(arena, size);
	ASSERT(subArena.memory, "Failed to allocate sub arena!");

	subArena.size = size;
	subArena.used = 0;

	return subArena;
}

TemporaryMemoryArena beginTemporaryMemory(MemoryArena *parentArena)
{
	ASSERT(!parentArena->hasTemporaryArenaOpen, "Beginning temporary memory without ending it!");

	TemporaryMemoryArena subArena = {};
	subArena.arena.size = (parentArena->size - parentArena->used);
	ASSERT(subArena.arena.size > 0, "No space for temporary memory!");
	ASSERT((parentArena->used + subArena.arena.size) <= parentArena->size, "Somehow temp memory is bigger than parent arena's free space!");
	subArena.arena.used = 0;
	subArena.arena.memory = (uint8*) parentArena->memory + parentArena->used;

	subArena.isOpen = true;
	subArena.parentArena = parentArena;
	parentArena->hasTemporaryArenaOpen = true;

	return subArena;
}

void endTemporaryMemory(TemporaryMemoryArena *tempArena)
{
	tempArena->isOpen = false;
	tempArena->parentArena->hasTemporaryArenaOpen = false;
}

#define PushStruct(Arena, Struct) ((Struct*)allocate(Arena, sizeof(Struct)))
#define PushArray(Arena, Type, Count) ((Type*)allocate(Arena, sizeof(Type) * Count))

#endif
