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
};

bool initMemoryArena(MemoryArena *arena, size_t size)
{
	bool succeeded = false;
	arena->memory = (uint8*)calloc(size, 1);
	if (arena->memory)
	{
		arena->size = size;
		arena->used = 0;
		succeeded = true;
	}
	
	return succeeded;
}

void *allocate(MemoryArena *arena, size_t size)
{
	ASSERT((arena->used + size) <= arena->size, "Arena out of memory!");
	
	void *result = arena->memory + arena->used;
	memset(result, 0, size);
	
	arena->used += size;
	
	return result;
}

/**
 * Allocate from the Arena, but without increasing the used counter. USE WITH CAUTION!
 */
void * tempAllocate(MemoryArena *arena, size_t size)
{
	ASSERT((arena->used + size) <= arena->size, "Arena out of memory!");
	
	void *result = arena->memory + arena->used;
	
	return result;
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

MemoryArena beginTemporaryMemory(MemoryArena *parentArena)
{
	MemoryArena subArena = {};
	subArena.size = (parentArena->size - parentArena->used);
	subArena.used = 0;
	subArena.memory = (uint8*) tempAllocate(parentArena, subArena.size);

	return subArena;
}

#define PushStruct(Arena, Struct) ((Struct*)allocate(Arena, sizeof(Struct)))
#define PushArray(Arena, Type, Count) ((Type*)allocate(Arena, sizeof(Type) * Count))

#endif
