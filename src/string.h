#pragma once

struct String
{
	char *chars;
	s32 length;
	s32 maxLength;
	bool isNullTerminated;

	char operator[](s32 index)
	{
		ASSERT(index >=0 && index < length, "String index out of range!");
		return chars[index];
	}
};

const String nullString = {};

inline String makeString(char *chars, s32 length)
{
	String result = {};
	result.chars = chars;
	result.length = length;
	result.maxLength = result.length;
	result.isNullTerminated = false;

	return result;
}
inline String makeString(char *chars)
{
	return makeString(chars, strlen(chars));
}
// const is a huge pain in the bum
inline String makeString(const char *chars)
{
	return makeString((char*)chars, strlen(chars));
}

void copyString(char *src, s32 srcLength, String *dest)
{
	s32 copyLength = MIN(srcLength, dest->maxLength);
	for (s32 i=0; i<copyLength; i++)
	{
		dest->chars[i] = src[i];
	}
	dest->length = copyLength;
}
inline void copyString(String src, String *dest)
{
	copyString(src.chars, src.length, dest);
}

String pushString(MemoryArena *arena, s32 length)
{
	String s = {};
	s.chars = PushArray(arena, char, length);
	s.length = 0;
	s.maxLength = length;

	return s;
}
String pushString(MemoryArena *arena, char *src)
{
	s32 len = strlen(src);

	String s = pushString(arena, len);
	copyString(src, len, &s);
	return s;
}
String pushString(MemoryArena *arena, String src)
{
	String s = pushString(arena, src.length);
	copyString(src, &s);
	return s;
}

bool equals(String a, String b)
{
	bool result = true;

	if (a.length != b.length)
	{
		result = false;
	}
	else
	{
		for (s32 i = 0; i<b.length; i++)
		{
			if (a.chars[i] != b.chars[i])
			{
				result = false;
				break;
			}
		}
	}

	return result;
}

inline bool equals(String a, char *b)
{
	return equals(a, makeString(b));
}

// Like strcmp()
s32 compare(String a, String b)
{
	bool foundDifference = false;
	s32 result = 0;
	for (s32 i = 0; i<b.length; i++)
	{
		s32 diff = a.chars[i] - b.chars[i];
		if (diff != 0)
		{
			result = diff;
			foundDifference = true;
			break;
		}
	}

	if (!foundDifference && a.length != b.length)
	{
		result = a.length - b.length;
	}

	return result;
}

u32 hashString(String s)
{
	// FNV-1a hash
	// http://www.isthe.com/chongo/tech/comp/fnv/

	u32 result = 2166136261;
	for (s32 i = 0; i < s.length; i++)
	{
		result ^= s.chars[i];
		result *= 16777619;
	}

	return result;
}

inline bool isWhitespace(unichar uChar, bool countNewlines=true)
{
	// TODO: There's probably more whitespace characters somewhere.

	bool result = false;

	// Feels like I'm misusing a switch, but I can't think of any better ways of writing this!
	switch (uChar)
	{
	case 0:
	case ' ':
	case '\t':
		result = true;
		break;

	case '\n':
	case '\r':
		result = countNewlines;
		break;

	default:
		result = false;
	}

	return result;
}

inline bool isNewline(char c)
{
	bool result = (c == '\n') || (c == '\r');
	return result;
}
