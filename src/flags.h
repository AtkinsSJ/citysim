#pragma once

//
// I've repeatedly found I want an enum that can be used as a set of flags, but also be usable as indices into an array.
// I've had to define the enums twice in this case, which is awkward. So, this is a flags field that takes a 0,1,2,3...N
// enum and turns it into a bitfield internally, using (1 << flag) to convert to a bit position.
// It's accessed the same way as a normal bitfield would be. & to check a flag, |= to add one and ^= to remove one.
// You can also use isEmpty() to check if anything is set, which is useful sometimes.
//
// A quirk of how it works is, you can't pass multiple flags at the same time. Maybe I'll change that later.
// But for now, only pure enum values will work, so we couldn't be able to muddle-up different enums by mistake.
//
// It's definitely not a replacement for ALL flag fields. But it's clearer when you see...
//     Flags<BuildingFlag> flags;
// ...than when you see...
//     u32 flags;
// ...and that should mean fewer chances for bugs.
//
// - Sam, 01/09/2019
//

template<typename Enum>
struct Flags
{
	s32 flagCount;
	u64 data;

	// Read the value of the flag
	bool operator&(Enum flag);

	// Add the flag
	Flags<Enum> *operator|=(Enum flag);

	// Remove the flag
	Flags<Enum> *operator^=(Enum flag);
};

template<typename Enum>
void initFlags(Flags<Enum> *flags, s32 flagCount);

template<typename Enum>
bool isEmpty(Flags<Enum> *flags);
