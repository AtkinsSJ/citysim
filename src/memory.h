#ifndef MEMORY_H
#define MEMORY_H

#include <stdlib.h> // For calloc

#define KB(x) ((x) * 1024)
#define MB(x) ((x) * 1024 * 1024)
#define GB(x) ((x) * 1024 * 1024 * 1024)

#define ArrayCount(a) (sizeof(a) / sizeof(a[0]))
#define ArrayCountS(a) ((int)(ArrayCount(a)))

struct memory_arena
{
	size_t Size;
	size_t Used;
	uint8 *Memory;
};

bool InitMemoryArena(memory_arena *Arena, size_t Size)
{
	bool Succeeded = false;
	Arena->Memory = (uint8*)calloc(Size, 1);
	if (Arena->Memory)
	{
		Arena->Size = Size;
		Arena->Used = 0;
		Succeeded = true;
	}
	
	return Succeeded;
}

void * Allocate(memory_arena *Arena, size_t Size)
{
	ASSERT((Arena->Used + Size) <= Arena->Size, "Arena out of memory!");
	
	void *Result = Arena->Memory + Arena->Used;
	
	Arena->Used += Size;
	
	return Result;
}

void ResetMemoryArena(memory_arena *Arena)
{
	Arena->Used = 0;
}

memory_arena AllocateSubArena(memory_arena *Arena, size_t Size)
{
	memory_arena SubArena = {};

	SubArena.Memory = (uint8 *)Allocate(Arena, Size);
	ASSERT(SubArena.Memory, "Failed to allocate sub arena!");

	SubArena.Size = Size;
	SubArena.Used = 0;

	return SubArena;
}

#define PushStruct(Arena, Struct) ((Struct*)Allocate(Arena, sizeof(Struct)))
#define PushArray(Arena, Type, Count) ((Type*)Allocate(Arena, sizeof(Type) * Count))

#endif
