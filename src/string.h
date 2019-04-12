#pragma once

struct String
{
	char *chars;
	s32 length;
	s32 maxLength;

	char operator[](s32 index)
	{
		ASSERT(index >=0 && index < length, "String index out of range!");
		return chars[index];
	}
};

const String nullString = {};

String newString(MemoryArena *arena, s32 length)
{
	String s = {};
	s.chars = PushArray(arena, char, length);
	s.length = 0;
	s.maxLength = length;

	return s;
}

inline String makeString(char *chars, s32 length)
{
	String result = {};
	result.chars = chars;
	result.length = length;
	result.maxLength = result.length;

	return result;
}

String stringFromChars(char *chars)
{
	String result = {};
	result.chars = chars;
	result.length = strlen(chars);
	result.maxLength = result.length;

	return result;
}
// const is a huge pain in the bum
String stringFromChars(const char *chars)
{
	return stringFromChars((char*)chars);
}

void copyChars(char *src, String *dest, s32 length)
{
	s32 copyLength = MIN(length, dest->maxLength);
	for (s32 i=0; i<copyLength; i++)
	{
		dest->chars[i] = src[i];
	}
	dest->length = copyLength;
}

String pushString(MemoryArena *arena, char *src)
{
	s32 len = strlen(src);

	String s = newString(arena, len);
	copyChars(src, &s, len);
	return s;
}

void copyString(String source, String *dest)
{
	s32 copyLength = MIN(source.length, dest->maxLength);
	for (s32 i=0; i<copyLength; i++)
	{
		dest->chars[i] = source.chars[i];
	}
	dest->length = copyLength;
}

String pushString(MemoryArena *arena, String src)
{
	String s = newString(arena, src.length);
	copyString(src, &s);
	return s;
}

void reverseString(char* first, u32 length)
{
	u32 flips = length / 2;
	char temp;
	for (u32 n=0; n < flips; n++)
	{
		temp = first[n];
		first[n] = first[length-1-n];
		first[length-1-n] = temp;
	}
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

bool equals(String a, char *b)
{
	return equals(a, stringFromChars(b));
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
