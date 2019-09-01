#pragma once

template<typename Enum>
bool Flags<Enum>::operator&(Enum flag)
{
	bool result = false;

	if (flag < 0 || flag >= this->flagCount)
	{
		ASSERT(false);
	}
	else
	{
		result = (this->data & ((u64)1 << flag)) != 0;
	}

	return result;
}

// Add the flag

template<typename Enum>
Flags<Enum> *Flags<Enum>::operator|=(Enum flag)
{
	this->data |= ((u64)1 << flag);
	return this;
}

// Remove the flag

template<typename Enum>
Flags<Enum> *Flags<Enum>::operator^=(Enum flag)
{
	u64 mask = ((u64)1 << flag);
	this->data &= ~mask;
	return this;
}

template<typename Enum>
void initFlags(Flags<Enum> *flags, s32 flagCount)
{
	ASSERT(flagCount < 64);

	flags->flagCount = flagCount;
	flags->data = 0;
}

template<typename Enum>
bool isEmpty(Flags<Enum> *flags)
{
	return (flags->data != 0);
}
