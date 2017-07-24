#pragma once

struct String
{
	char *chars;
	int32 length;
};

String newString(MemoryArena *arena, int32 length)
{
	String s = {};
	s.chars = PushArray(arena, char, length);
	s.length = length;

	return s;
}

// inline String copyString(char *chars, int32 length)
// {
// 	String result = {};
// 	result.chars = PushArray(globalFrameTempArena, char, length);
// 	result.length = length;

// 	for (int32 i=0; i<length; i++)
// 	{
// 		result.chars[i] = chars[i];
// 	}

// 	return result;
// }

inline String makeString(char *chars, int32 length)
{
	String result = {};
	result.chars = chars;
	result.length = length;

	return result;
}

String stringFromChars(char *chars)
{
	String result = {};
	result.chars = chars;
	result.length = strlen(chars);

	return result;
}

struct StringBuffer
{
	char *buffer;
	int32 length;
	int32 maxLength;

	int32 caretPos;
};

StringBuffer newStringBuffer(MemoryArena *arena, int32 length)
{
	StringBuffer b = {};
	b.buffer = PushArray(arena, char, length + 1);
	b.maxLength = length;

	return b;
}

String bufferToString(StringBuffer *buffer)
{
	String result = {};
	result.chars = buffer->buffer;
	result.length = buffer->length;

	return result;
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

void append(StringBuffer *buffer, char *source, int32 length)
{
	int32 lengthToCopy = length;
	if ((buffer->length + length) > buffer->maxLength)
	{
		lengthToCopy = buffer->maxLength - buffer->length;
	}

	for (int32 i=0; i < lengthToCopy; i++)
	{
		buffer->buffer[buffer->length++] = source[i];
	}

	// Final null, just in case.
	buffer->buffer[buffer->length] = 0;
	buffer->caretPos += lengthToCopy;
}

void append(StringBuffer *buffer, String source)
{
	append(buffer, source.chars, source.length);
}

void append(StringBuffer *buffer, char *source)
{
	append(buffer, stringFromChars(source));
}

void append(StringBuffer *buffer, StringBuffer *source)
{
	append(buffer, source->buffer, source->length);
}

void insert(StringBuffer *buffer, char *source, int32 length)
{
	if (buffer->caretPos == buffer->length)
	{
		append(buffer, source, length);
		return;
	}

	int32 lengthToCopy = length;
	if ((buffer->length + length) > buffer->maxLength)
	{
		lengthToCopy = buffer->maxLength - buffer->length;
	}

	// move the existing chars by lengthToCopy
	for (int32 i=buffer->length - buffer->caretPos - 1; i>=0; i--)
	{
		buffer->buffer[buffer->caretPos + lengthToCopy + i] = buffer->buffer[buffer->caretPos + i];
	}

	// write from source
	for (int32 i=0; i < lengthToCopy; i++)
	{
		buffer->buffer[buffer->caretPos + i] = source[i];
	}

	buffer->length += lengthToCopy;
	buffer->caretPos += lengthToCopy;
}

void backspace(StringBuffer *buffer)
{
	if (buffer->caretPos > 0) 
	{
		buffer->caretPos--;

		// copy everything 1 to the left
		for (int32 i=buffer->caretPos; i<buffer->length-1; i++)
		{
			buffer->buffer[i] = buffer->buffer[i+1];
		}

		buffer->buffer[--buffer->length] = 0;
	}
}

void deleteChar(StringBuffer *buffer)
{
	if (buffer->caretPos < buffer->length) 
	{
		// copy everything 1 to the left
		for (int32 i=buffer->caretPos; i<buffer->length-1; i++)
		{
			buffer->buffer[i] = buffer->buffer[i+1];
		}

		buffer->buffer[--buffer->length] = 0;
	}
}

void clear(StringBuffer *buffer)
{
	buffer->length = 0;
	buffer->buffer[0] = 0;
	buffer->caretPos = 0;
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

bool equals(StringBuffer *buffer, char *other)
{
	return equals(bufferToString(buffer), stringFromChars(other));
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

bool isWhitespace(uint32 uChar)
{
	// TODO: FINISH THIS!

	bool result = false;

	switch (uChar)
	{
	case 0:
	case 32:
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

TokenList tokenize(String input)
{
	TokenList result = {};
	int32 position = 0;

	while (position <= input.length)
	{
		while ((position <= input.length) && isWhitespace(input.chars[position]))
		{
			position++;
		}

		if (position <= input.length)
		{
			ASSERT(result.count < result.maxTokenCount, "No room for more tokens.");
			String *token = &result.tokens[result.count++];
			token->chars = input.chars + position;
			
			// length
			while ((position <= input.length) && !isWhitespace(input.chars[position]))
			{
				position++;
				token->length++;
			}
		}
	}

	return result;
}

#include "string.cpp"