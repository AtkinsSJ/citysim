#pragma once

struct String
{
	char *chars;
	int32 length;
	int32 maxLength;
};

const String nullString = {};

String newString(MemoryArena *arena, int32 length)
{
	String s = {};
	s.chars = PushArray(arena, char, length);
	s.length = 0;
	s.maxLength = length;

	return s;
}

inline String makeString(char *chars, int32 length)
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

void copyString(String source, String *dest)
{
	int32 copyLength = MIN(source.length, dest->maxLength);
	for (int32 i=0; i<copyLength; i++)
	{
		dest->chars[i] = source.chars[i];
	}
	dest->length = copyLength;
}

void reverseString(char* first, uint32 length)
{
	uint32 flips = length / 2;
	char temp;
	for (uint32 n=0; n < flips; n++)
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
		for (int32 i = 0; i<b.length; i++)
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

bool asInt(String string, int64 *result)
{
	bool succeeded = true;

	int64 value = 0;
	int32 startPosition = 0;
	bool isNegative = false;
	if (string.chars[0] == '-')
	{
		isNegative = true;
		startPosition++;
	}
	else if (string.chars[0] == '+')
	{
		// allow a leading + in case people want it for some reason.
		startPosition++;
	}

	for (int position=startPosition; position<string.length; position++)
	{
		value *= 10;

		char c = string.chars[position];
		if (c >= '0' && c <= '9')
		{
			value += c - '0';
		}
		else
		{
			succeeded = false;
			break;
		}
	}

	if (succeeded)
	{
		*result = value;
		if (isNegative) *result = -*result;
	}

	return succeeded;
}

bool isWhitespace(unichar uChar)
{
	// TODO: There's probably more whitespace characters somewhere.

	bool result = false;

	// Feels like I'm misusing a switch, but I can't think of any better ways of writing this!
	switch (uChar)
	{
	case 0:
	case ' ':
	case '\n':
	case '\r':
	case '\t':
		result = true;
		break;

	default:
		result = false;
	}

	return result;
}

struct TokenList
{
	String tokens[64];
	int32 count;
	int32 maxTokenCount = 64;
};

// TODO: This probably just wants to be "getToken()" which splits the string into two parts.
// That way, no need to worry in advance how many tokens to make room for.
// It also means we can use the rest of the string, including spaces, if we want!
TokenList tokenize(String input)
{
	TokenList result = {};
	int32 position = 0;

	while (position < input.length)
	{
		while ((position <= input.length) && isWhitespace(input.chars[position]))
		{
			position++;
		}

		if (position < input.length)
		{
			ASSERT(result.count < result.maxTokenCount, "No room for more tokens.");
			String *token = &result.tokens[result.count++];
			token->chars = input.chars + position;
			
			// length
			while ((position < input.length) && !isWhitespace(input.chars[position]))
			{
				position++;
				token->length++;
			}
		}
	}

	return result;
}

#include "unicode.h"

#include "stringbuilder.h"

#include "string.cpp"