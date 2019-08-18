
void initBitField(BitField *bitField, MemoryArena *arena, s32 size)
{
	bitField->size = size;
	bitField->setBitsCount = 0;

	// I can't think of a good name for this, but it's how many u64s we need
	s32 fieldsCount = size / 64;
	if (fieldsCount <= 0) fieldsCount = 1;

	bitField->bits = allocateArray<u64>(arena, fieldsCount);
}

inline bool BitField::operator[](s32 index)
{
	bool result = false;

	if (index >= this->size || index < 0)
	{
		ASSERT(false);
	}
	else
	{
		// @Speed: Could do these with bitshifts, once I know it's working right
		s32 fieldIndex = index / 64;
		u64 bitIndex = index % 64;

		u64 mask = ((u64)1 << bitIndex);
		result = (this->bits[fieldIndex] & mask) != 0;
	}

	return result;
}

void setBit(BitField *bitField, s32 index, bool value)
{
	if (index >= bitField->size || index < 0)
	{
		ASSERT(false);
	}
	else
	{
		// @Speed: Could do these with bitshifts, once I know it's working right
		s32 fieldIndex = index / 64;
		u64 bitIndex = index % 64;
		
		u64 mask = ((u64)1 << bitIndex);

		bool wasSet = (bitField->bits[fieldIndex] & mask) != 0;

		if (wasSet != value)
		{
			if (value)
			{
				bitField->bits[fieldIndex] |= mask;
				bitField->setBitsCount++;
			}
			else
			{
				bitField->bits[fieldIndex] &= ~mask;
				bitField->setBitsCount--;
			}
		}
	}
}

void clearBits(BitField *bitField)
{
	bitField->setBitsCount = 0;

	for (s32 i = 0; i < bitField->bits.count; i++)
	{
		bitField->bits[i] = 0;
	}
}
