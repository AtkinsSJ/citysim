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

inline char & String::operator[](s32 index)
{
	ASSERT(index >=0 && index < this->length); //Index out of range!
	return this->chars[index];
}

void copyString(char *src, s32 srcLength, String *dest)
{
	s32 copyLength = min(srcLength, dest->maxLength);
	copyMemory(src, dest->chars, copyLength);
	dest->length = copyLength;
}

inline void copyString(String src, String *dest)
{
	copyString(src.chars, src.length, dest);
}

inline String pushString(MemoryArena *arena, s32 length, bool setLength)
{
	String s = {};
	s.chars = allocateMultiple<char>(arena, length);
	s.length = setLength ? length : 0;
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

inline bool stringIsValid(String s)
{
	bool isValid = true;

	// NB: The final char (length-1) is allowed to be null, so we only check until the one before that
	for (s32 index=0; index < s.length-1; index++)
	{
		if (s.chars[index] == 0)
		{
			isValid = false;
			break;
		}
	}

	return isValid;
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
	while (!isEmpty(input) && isWhitespace(result.chars[0], false))
	{
		++result.chars;
		--result.length;
	}

	return result;
}

String trimEnd(String input)
{
	String result = input;
	while (!isEmpty(input) && isWhitespace(result.chars[result.length-1], false))
	{
		--result.length;
	}

	return result;
}

inline String trim(String input)
{
	return trimStart(trimEnd(input));
}

Maybe<s64> asInt(String input)
{
	DEBUG_FUNCTION();

	Maybe<s64> result = makeFailure<s64>();
	
	bool succeeded = input.length > 0;

	if (succeeded)
	{
		s64 value = 0;
		s32 startPosition = 0;
		bool isNegative = false;
		if (input.chars[0] == '-')
		{
			isNegative = true;
			startPosition++;
		}
		else if (input.chars[0] == '+')
		{
			// allow a leading + in case people want it for some reason.
			startPosition++;
		}

		for (s32 position=startPosition; position<input.length; position++)
		{
			value *= 10;

			char c = input.chars[position];
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
			if (isNegative)
			{
				result = makeSuccess(-value);
			}
			else
			{
				result = makeSuccess(value);
			}
		}
	}

	return result;
}

Maybe<f64> asFloat(String input)
{
	Maybe<f64> result;

	// TODO: Implement this properly!
	// (c runtime functions atof / strtod don't tell you if they failed, they just return 0 which is a valid value!
	String nullTerminatedInput = pushString(tempArena, input.length+1);
	copyString(input, &nullTerminatedInput);
	nullTerminatedInput.length++;
	nullTerminatedInput[input.length] = '\0';

	f64 doubleValue = atof(nullTerminatedInput.chars);
	if (doubleValue == 0.0)
	{
		// @Hack: 0.0 is returned by atof() if it fails. So, we see if the input really began with a '0' or not.
		// If it didn't, we assume it failed. If it did, we assume it succeeded.
		// Note that if it failed on a value beginning with a '0' character, (eg "0_0") then it will assume it succeeded...
		// But, I don't know a more reliable way without just parsing the float value ourselves so eh!
		// - Sam, 12/09/2019
		if (input[0] == '0')
		{
			result = makeSuccess(doubleValue);
		}
		else
		{
			result = makeFailure<f64>();
		}
	}
	else
	{
		result = makeSuccess(doubleValue);
	}

	return result;
}

Maybe<bool> asBool(String input)
{
	DEBUG_FUNCTION();

	Maybe<bool> result;

	if (equals(input, "true"_s))
	{
		result = makeSuccess(true);
	}
	else if (equals(input, "false"_s))
	{
		result = makeSuccess(false);
	}
	else
	{
		result = makeFailure<bool>();
	}

	return result;
}

inline bool isNullTerminated(String s)
{
	// A 0-length string, by definition, can't have a null terminator
	bool result = (s.length > 0) && (s.chars[s.length-1] == 0);
	return result;
}

inline bool isEmpty(String s)
{
	return (s.length <= 0);
}

bool endsWith(String s, String suffix)
{
	bool result = false;

	if (s.length == suffix.length)
	{
		result = equals(s, suffix);
	}
	else if (s.length > suffix.length)
	{
		result = isMemoryEqual<char>(suffix.chars, (s.chars + s.length - suffix.length), suffix.length);
	}
	else
	{
		// Otherwise, suffix > s, so it can't end with it!
		result = false;
	}

	return result;
}

Maybe<s32> findIndexOfChar(String input, char c, bool searchFromEnd, s32 startIndex)
{
	Maybe<s32> result;

	if (isEmpty(input))
	{
		result = makeFailure<s32>();
	}
	else
	{
		if (startIndex == -1)
		{
			startIndex = (searchFromEnd ? input.length - 1 : 0);
		}

		s32 pos = startIndex;
		bool found = false;
		
		if (searchFromEnd)
		{
			while (pos >= 0)
			{
				if (input[pos] == c)
				{
					found = true;
					break;
				}

				pos--;
			}
		}
		else
		{
			while (pos < input.length)
			{
				if (input[pos] == c)
				{
					found = true;
					break;
				}

				pos++;
			}
		}

		if (found)
		{
			result = makeSuccess(pos);
		}
		else
		{
			result = makeFailure<s32>();
		}
	}

	return result;
}

bool contains(String input, char c)
{
	return findIndexOfChar(input, c, false).isValid;
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

String concatenate(std::initializer_list<String> strings, String between)
{
	bool useBetween = (between.length > 0);
	String result = nullString;

	if (strings.size() > 0)
	{
		// Count up the resulting length
		s32 resultLength = truncate32(between.length * (strings.size() - 1));
		for (auto it = strings.begin(); it != strings.end(); it++)
		{
			resultLength += it->length;
		}

		StringBuilder stb = newStringBuilder(resultLength, tempArena);

		append(&stb, *strings.begin());

		for (auto it = (strings.begin() + 1); it != strings.end(); it++)
		{
			if (useBetween) append(&stb, between);
			append(&stb, *it);
		}

		result = getString(&stb);
	}

	return result;
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

	// Null bytes are a pain.
	// If we have a terminating one, just trim it off for now (and optionally re-attach it
	// based on the zeroTerminate param.
	// Any nulls in-line will just be printed to the result, following the "garbage in, garbage out"
	// principle!
	if (isNullTerminated(format))
	{
		format.length--;
	}

	StringBuilder stb = newStringBuilder(format.length * 4);

	for (s32 i=0; i<format.length; i++)
	{
		if (format.chars[i] == '{')
		{
			i++; // Skip the {

			s32 startOfNumber = i;

			// Run until the next character is a } or we're done
			while (((i+1) < format.length)
				&& (format.chars[i] != '}'))
			{
				i++;
			}

			String indexString = makeString(format.chars + startOfNumber, (i - startOfNumber));
			Maybe<s64> parsedIndex = asInt(indexString);
			if ((parsedIndex.isValid)
			 && (parsedIndex.value >= 0 && parsedIndex.value < (s64)args.size()))
			{
				String arg = args.begin()[parsedIndex.value];

				if (isNullTerminated(arg)) arg.length--;

				append(&stb, arg);
			}
			else
			{
				// If the index is invalid, show some kind of error. For now, we'll just insert the {n} as given.
				append(&stb, '{');
				append(&stb, indexString);
				append(&stb, '}');
			}
		}
		else
		{
			s32 startIndex = i;

			// Run until the next character is a { or we're done
			while (((i+1) < format.length)
				&& (format.chars[i + 1] != '{'))
			{
				i++;
			}

			append(&stb, format.chars + startIndex, i + 1 - startIndex);
		}
	}

	if (zeroTerminate)
	{
		append(&stb, '\0');
	}

	String result = getString(&stb);

	return result;
}


const char* const intBaseChars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
String formatInt(u64 value, u8 base, s32 zeroPadWidth)
{
	DEBUG_FUNCTION();
	
	ASSERT((base > 1) && (base <= 36)); //formatInt() only handles base 2 to base 36.
	s32 arraySize = max(64, zeroPadWidth);
	char *temp = allocateMultiple<char>(tempArena, arraySize); // Worst case is base 1, which is 64 characters!
	s32 count = 0;

	u64 v = value;

	do
	{
		// We start at the end and work backwards, so that we don't have to reverse the string afterwards!
		temp[arraySize - 1 - count++] = intBaseChars[v % base];
		v = v / base;
	}
	while (v != 0);

	while (count < zeroPadWidth)
	{
		temp[arraySize - 1 - count++] = '0';
	}

	return makeString(temp + (arraySize - count), count);
}

String formatInt(s64 value, u8 base, s32 zeroPadWidth)
{
	DEBUG_FUNCTION();
	
	ASSERT((base > 1) && (base <= 36)); //formatInt() only handles base 2 to base 36.
	s32 arraySize = max(65, zeroPadWidth+1);
	char *temp = allocateMultiple<char>(tempArena, arraySize); // Worst case is base 1, which is 64 characters! Plus 1 for sign
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

	while (count < zeroPadWidth)
	{
		temp[arraySize - 1 - count++] = '0';
	}

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
	
	String formatString = myprintf("%.{0}f"_s, {formatInt(decimalPlaces)}, true);

	s32 length = 100; // TODO: is 100 enough?
	char *buffer = allocateMultiple<char>(tempArena, length);
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
		String result = pushString(tempArena, length);
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
	if (value) return "true"_s;
	else       return "false"_s;
}

String repeatChar(char c, s32 length)
{
	String result = pushString(tempArena, length);
	result.length = length;

	for (s32 i=0; i<length; i++)
	{
		result.chars[i] = c;
	}

	return result;
}
