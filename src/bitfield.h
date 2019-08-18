#pragma once

struct BitField
{
	s32 size;
	s32 setBitsCount;

	Array<u64> bits;

	bool operator[](s32 index);
};

void initBitField(BitField *bitField, MemoryArena *arena, s32 size);

void setBit(BitField *bitField, s32 index, bool value);

void clearBits(BitField *bitField);
