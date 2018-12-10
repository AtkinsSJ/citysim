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

bool asInt(String string, s64 *result)
{
	bool succeeded = true;

	s64 value = 0;
	s32 startPosition = 0;
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

bool asBool(String string, bool *result)
{
	bool succeeded = true;

	if (equals(string, "true"))
	{
		*result = true;
	}
	else if (equals(string, "false"))
	{
		*result = false;
	}
	else
	{
		succeeded = false;
	}

	return succeeded;
}

bool isWhitespace(unichar uChar, bool countNewlines=true)
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

bool isNewline(char c)
{
	bool result = (c == '\n') || (c == '\r');
	return result;
}

struct TokenList
{
	String tokens[64];
	s32 count;
	s32 maxTokenCount = 64;
};

// TODO: This probably just wants to be "getToken()" which splits the string into two parts.
// That way, no need to worry in advance how many tokens to make room for.
// It also means we can use the rest of the string, including spaces, if we want!
TokenList tokenize(String input)
{
	TokenList result = {};
	s32 position = 0;

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

String trimStart(String input)
{
	String result = input;
	while ((input.length > 0) && isWhitespace(result.chars[0], false))
	{
		++result.chars;
		--result.length;
	}

	return result;
}

String trimEnd(String input)
{
	String result = input;
	while ((input.length > 0) && isWhitespace(result.chars[result.length-1], false))
	{
		--result.length;
	}

	return result;
}

String trim(String input)
{
	return trimStart(trimEnd(input));
}

String nextToken(String input, String *remainder)
{
	String firstWord = input;
	firstWord.length = 0;

	while (!isWhitespace(firstWord.chars[firstWord.length], true)
		&& (firstWord.length < input.length))
	{
		++firstWord.length;
	}

	if (remainder)
	{
		remainder->chars = firstWord.chars + firstWord.length;
		remainder->length = input.length - firstWord.length;
		*remainder = trimStart(*remainder);
	}

	return firstWord;
}

bool splitInTwo(String input, char divider, String *leftResult, String *rightResult)
{
	bool foundDivider = false;

	for (s32 i=0; i < input.length; i++)
	{
		if (input[i] == divider)
		{
			leftResult->chars = input.chars;
			leftResult->length = i;

			rightResult->chars = input.chars + i + 1;
			rightResult->length = input.length - i - 1;

			foundDivider = true;
			break;
		}
	}

	return foundDivider;
}