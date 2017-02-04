#pragma once

struct String
{
	char *chars;
	int32 length;
};

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
	int32 bufferLength;
	int32 bufferMaxLength;
};

StringBuffer newStringBuffer(MemoryArena *arena, int32 length)
{
	StringBuffer b = {};
	b.buffer = PushArray(arena, char, length + 1);
	b.bufferMaxLength = length;

	return b;
}

String bufferToString(StringBuffer *buffer)
{
	String result = {};
	result.chars = buffer->buffer;
	result.length = buffer->bufferLength;

	return result;
}

void append(StringBuffer *buffer, char *source, int32 length)
{
	int32 lengthToCopy = length;
	if ((buffer->bufferLength + length) > buffer->bufferMaxLength)
	{
		lengthToCopy = buffer->bufferMaxLength - buffer->bufferLength;
	}

	for (int32 i=0; i < lengthToCopy; i++)
	{
		buffer->buffer[buffer->bufferLength++] = source[i];
	}

	// Final null, just in case.
	buffer->buffer[buffer->bufferLength] = 0;
}

void append(StringBuffer *buffer, String source)
{
	append(buffer, source.chars, source.length);
}

void append(StringBuffer *buffer, char *source)
{
	append(buffer, stringFromChars(source));
}

void append(StringBuffer *dest, StringBuffer *src)
{
	append(dest, src->buffer, src->bufferLength);
}

void backspace(StringBuffer *buffer)
{
	if (buffer->bufferLength > 0)
	{
		buffer->buffer[--buffer->bufferLength] = 0;
	}
}

void clear(StringBuffer *buffer)
{
	buffer->bufferLength = 0;
	buffer->buffer[0] = 0;
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

bool asInt(String string, int32 *result)
{
	bool succeeded = true;

	int32 value = 0;
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