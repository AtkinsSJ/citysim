#pragma once
#include <stdio.h> // For snprintf

inline String makeString(char *chars, s32 length, bool hash)
{
	String result = {};
	result.chars = chars;
	result.length = length;
	result.maxLength = result.length;
	result.hasHash = false;
	result.hash = 0;

	if (hash)
	{
		hashString(&result); // NB: Sets the hash and hasHash.
	}

	return result;
}
inline String makeString(char *chars, bool hash)
{
	return makeString(chars, truncate32(strlen(chars)), hash);
}
// const is a huge pain in the bum
inline String makeString(const char *chars, bool hash)
{
	return makeString((char*)chars, truncate32(strlen(chars)), hash);
}

inline String stringFromBlob(Blob blob, bool hash)
{
	return makeString((char*)blob.memory, truncate32(blob.size), hash);
}

void copyString(char *src, s32 srcLength, String *dest)
{
	DEBUG_FUNCTION();
	
	s32 copyLength = min(srcLength, dest->maxLength);
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

inline String pushString(MemoryArena *arena, s32 length)
{
	String s = {};
	s.chars = allocateArray<char>(arena, length);
	s.length = 0;
	s.maxLength = length;

	return s;
}

inline String pushString(MemoryArena *arena, char *src)
{
	s32 len = truncate32(strlen(src));

	String s = pushString(arena, len);
	copyString(src, len, &s);
	return s;
}

inline String pushString(MemoryArena *arena, String src)
{
	String s = pushString(arena, src.length);
	copyString(src, &s);
	return s;
}

inline bool equals(String a, String b)
{
	bool result = true;

	if (a.length != b.length)
	{
		result = false;
	}
	else if (a.hasHash && b.hasHash && a.hash != b.hash)
	{
		result = false;
	}
	else
	{
		result = isMemoryEqual(a.chars, b.chars, a.length);
	}

	return result;
}

inline bool equals(String a, char *b)
{
	return equals(a, makeString(b));
}

u32 hashString(String *s)
{
	// DEBUG_FUNCTION(); // Can't, because we use String hashes in the debug system!

	u32 result = s->hash;

	if (!s->hasHash)
	{
		// FNV-1a hash
		// http://www.isthe.com/chongo/tech/comp/fnv/
		result = 2166136261;
		for (s32 i = 0; i < s->length; i++)
		{
			result ^= s->chars[i];
			result *= 16777619;
		}

		s->hash = result;
		s->hasHash = true;
	}

	return result;
}

void reverse(char* first, u32 length)
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

inline String trim(String input)
{
	return trimStart(trimEnd(input));
}

bool asInt(String string, s64 *result)
{
	DEBUG_FUNCTION();
	
	bool succeeded = string.length > 0;

	if (succeeded)
	{
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
	}

	return succeeded;
}

bool asBool(String string, bool *result)
{
	DEBUG_FUNCTION();
	
	bool succeeded = string.length > 0;

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

inline bool isNullTerminated(String s)
{
	// A 0-length string, by definition, can't have a null terminator
	bool result = (s.length > 0) && (s.chars[s.length-1] == 0);
	return result;
}

inline bool isEmpty(String s)
{
	return (s.length == 0);
}

s32 countTokens(String input)
{
	DEBUG_FUNCTION();
	
	s32 result = 0;

	s32 position = 0;

	while (position < input.length)
	{
		while ((position <= input.length) && isWhitespace(input.chars[position]))
		{
			position++;
		}

		if (position < input.length)
		{
			result++;
			
			// length
			while ((position < input.length) && !isWhitespace(input.chars[position]))
			{
				position++;
			}
		}
	}

	return result;
}

// If splitChar is provided, the token ends before that, and it is skipped.
// Otherwise, we stop at the first whitespace character, determined by isWhitespace()
String nextToken(String input, String *remainder, char splitChar)
{
	DEBUG_FUNCTION();
	
	String firstWord = input;
	firstWord.length = 0;

	if (splitChar == 0)
	{
		while (!isWhitespace(firstWord.chars[firstWord.length], true)
			&& (firstWord.length < input.length))
		{
			++firstWord.length;
		}
	}
	else
	{
		while (firstWord.chars[firstWord.length] != splitChar
			&& (firstWord.length < input.length))
		{
			++firstWord.length;
		}

		firstWord = trim(firstWord);
	}

	firstWord.maxLength = firstWord.length;

	if (remainder)
	{
		// NB: We have to make sure we properly initialise remainder here, because we had a bug before
		// where we didn't, and it sometimes had old data in the "hasHash" field, which was causing all
		// kinds of weird stuff to happen!
		*remainder = trimStart(makeString(firstWord.chars + firstWord.length, input.length - firstWord.length));

		// Skip the split char
		if (splitChar != 0 && remainder->length > 0)
		{
			remainder->length--;
			remainder->chars++;
			*remainder = trimStart(*remainder);
		}

		remainder->maxLength = remainder->length;
	}

	return firstWord;
}

// NB: You can pass null for leftResult or rightResult to ignore that part.
bool splitInTwo(String input, char divider, String *leftResult, String *rightResult)
{
	DEBUG_FUNCTION();
	
	bool foundDivider = false;

	for (s32 i=0; i < input.length; i++)
	{
		if (input.chars[i] == divider)
		{
			// NB: We have to make sure we properly initialise leftResult/rightResult here, because we had a
			// bug before where we didn't, and it sometimes had old data in the "hasHash" field, which was
			// causing all kinds of weird stuff to happen!
			// Literally the exact same issue I fixed in nextToken() yesterday. /fp
			if (leftResult != null)
			{
				*leftResult = makeString(input.chars, i);
			}

			if (rightResult != null)
			{
				*rightResult = makeString(input.chars + i + 1, input.length - i - 1);
			}

			foundDivider = true;
			break;
		}
	}

	return foundDivider;
}

/**
 * A printf() that takes a string like "Hello {0}!" where each {n} is replaced by the arg at that index.
 * Extra { and } characters may be stripped. We try to print out any invalid {n} indices but there is not a ton
 * of error-reporting. Try to pass valid input!
 * You pass the arguments in an initializer-list, so like: myprintf("Hello {0}!", {name});
 * All arguments must already be Strings. Use the various formatXXX() functions to convert things to Strings.
 */
String myprintf(String format, std::initializer_list<String> args, bool zeroTerminate)
{
	DEBUG_FUNCTION();
	
	String result;

	StringBuilder stb = newStringBuilder(format.length * 4);

	s32 startOfNumber = s32Max;
	bool isReadingNumber = false;
	for (s32 i=0; i<format.length; i++)
	{
		switch (format.chars[i])
		{
			case '{': {
				startOfNumber = i+1;
				isReadingNumber = true;
			} break;

			case '}': {
				s32 endOfNumber = i;
				bool succeeded = false;
				if (isReadingNumber && endOfNumber > startOfNumber)
				{
					String indexString = makeString(format.chars + startOfNumber, (endOfNumber - startOfNumber));
					s64 parsedIndex = 0;
					if (asInt(indexString, &parsedIndex))
					{
						// now we try and see if it's valid
						if (parsedIndex >= 0 && parsedIndex < (s64)args.size())
						{
							succeeded = true;
							String arg = args.begin()[parsedIndex];

							// We don't want the null termination byte to be included in the length, or else we get problems if
							// we myprintf() the result! Yes, this has happened. It was confusing.
							// - Sam, 10/12/2018
							// Update 14/05/2019 - we now do the check here, instead of myprintf() appending a null byte that's not included in the String length. (Because checking if that byte is there for isNullTerminated() means reading off the end of the array... not a great idea! 
							if (isNullTerminated(arg)) arg.length--;

							append(&stb, arg);
						}
						
					}

					if (!succeeded)
					{
						// If the index is invalid, show some kind of error. For now, we'll just insert the {n} as given.
						append(&stb, "{");
						append(&stb, indexString);
						append(&stb, "}");
					}
				}

				isReadingNumber = false;
				startOfNumber = s32Max;
			} break;

			default: {
				if (!isReadingNumber)
				{
					char c = format.chars[i];
					// We keep getting bugs where I accidentally include null bytes, so remove them!
					if (c) append(&stb, c);
				}
			}
		}
	}

	if (zeroTerminate)
	{
		append(&stb, '\0');
	}

	result = getString(&stb);

	return result;
}


const char* const intBaseChars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
String formatInt(u64 value, u8 base)
{
	DEBUG_FUNCTION();
	
	ASSERT((base > 1) && (base <= 36)); //formatInt() only handles base 2 to base 36.
	s32 arraySize = 64;
	char *temp = allocateArray<char>(globalFrameTempArena, arraySize); // Worst case is base 1, which is 64 characters!
	s32 count = 0;

	u64 v = value;

	do
	{
		// We start at the end and work backwards, so that we don't have to reverse the string afterwards!
		temp[arraySize - 1 - count++] = intBaseChars[v % base];
		v = v / base;
	}
	while (v != 0);

	return makeString(temp + (arraySize - count), count);
}

String formatInt(s64 value, u8 base)
{
	DEBUG_FUNCTION();
	
	ASSERT((base > 1) && (base <= 36)); //formatInt() only handles base 2 to base 36.
	s32 arraySize = 65;
	char *temp = allocateArray<char>(globalFrameTempArena, arraySize); // Worst case is base 1, which is 64 characters! Plus 1 for sign
	bool isNegative = (value < 0);
	s32 count = 0;

	// One complication here: If we're passed s64_MIN, then -value is 1 larger than can be held in an s64!
	// So, rather than flipping it and treating it like a positive number with an '-' appended,
	// we have to make each digit positive as we get it.

	// s64 v = isNegative ? -value : value;
	s64 v = value;

	do
	{
		temp[arraySize - 1 - count++] = intBaseChars[ ((isNegative ? -1 : 1) * (v % base)) ];
		v = v / base;
	}
	while (v != 0);

	if (isNegative)
	{
		temp[arraySize - 1 - count++] = '-';
	}

	return makeString(temp + (arraySize - count), count);
}

// TODO: formatFloat() is a total trainwreck, we should really do this a lot better!
// Whether we use our own float-to-string routine or not, we definitely don't want to
// continue calling myprintf() to produce a string for snprintf() and then pass that
// on to myprintf() again! Like, that's super dumb.
String formatFloat(f64 value, s32 decimalPlaces)
{
	DEBUG_FUNCTION();
	
	String formatString = myprintf("%.{0}f\0", {formatInt(decimalPlaces)});

	s32 length = 100; // TODO: is 100 enough?
	char *buffer = allocateArray<char>(globalFrameTempArena, length);
	s32 written = snprintf(buffer, length, formatString.chars, value);

	return makeString(buffer, min(written, length));
}

String formatString(String value, s32 length, bool alignLeft, char paddingChar)
{
	DEBUG_FUNCTION();
	
	if ((value.length == length) || (length == -1)) return value;

	if (length < value.length)
	{
		String result = makeString(value.chars, length);
		return result;
	}
	else
	{
		String result = pushString(globalFrameTempArena, length);
		result.length = length;

		if (alignLeft)
		{
			for (s32 i=0; i<value.length; i++)
			{
				result.chars[i] = value.chars[i];
			}
			for (s32 i=value.length; i<length; i++)
			{
				result.chars[i] = paddingChar;
			}
		}
		else // alignRight
		{
			s32 startPos = length - value.length;
			for (s32 i=0; i<value.length; i++)
			{
				result.chars[i + startPos] = value.chars[i];
			}
			for (s32 i=0; i<startPos; i++)
			{
				result.chars[i] = paddingChar;
			}
		}

		return result;
	}
}

String formatBool(bool value)
{
	if (value) return makeString("true");
	else       return makeString("false");
}

String repeatChar(char c, s32 length)
{
	String result = pushString(globalFrameTempArena, length);
	result.length = length;

	for (s32 i=0; i<length; i++)
	{
		result.chars[i] = c;
	}

	return result;
}
