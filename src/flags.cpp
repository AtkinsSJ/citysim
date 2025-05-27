#pragma once

template<typename Enum, typename Storage>
bool Flags<Enum, Storage>::operator&(Enum flag)
{
    bool result = false;

    if (flag < 0 || flag >= this->flagCount) {
        ASSERT(false);
    } else {
        result = (this->data & ((u64)1 << flag)) != 0;
    }

    return result;
}

// Add the flag

template<typename Enum, typename Storage>
Flags<Enum, Storage>* Flags<Enum, Storage>::operator+=(Enum flag)
{
    this->data |= ((u64)1 << flag);
    return this;
}

// Remove the flag

template<typename Enum, typename Storage>
Flags<Enum, Storage>* Flags<Enum, Storage>::operator-=(Enum flag)
{
    u64 mask = ((u64)1 << flag);
    this->data &= ~mask;
    return this;
}

// Toggle the flag
template<typename Enum, typename Storage>
Flags<Enum, Storage>* Flags<Enum, Storage>::operator^=(Enum flag)
{
    this->data ^= ((u64)1 << flag);
    return this;
}

template<typename Enum, typename Storage>
bool Flags<Enum, Storage>::operator==(Flags<Enum, Storage>& other)
{
    return this->data == other.data;
}

template<typename Enum, typename Storage>
void initFlags(Flags<Enum, Storage>* flags, Enum flagCount)
{
    ASSERT(flagCount <= (8 * sizeof(Storage)));

    flags->flagCount = flagCount;
    flags->data = 0;
}

template<typename Enum, typename Storage>
bool isEmpty(Flags<Enum, Storage>* flags)
{
    return (flags->data == 0);
}

template<typename Enum, typename Storage>
Storage getAll(Flags<Enum, Storage>* flags)
{
    return flags->data;
}
