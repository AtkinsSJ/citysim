#pragma once
#include <stdio.h> // For snprintf

/**
 * A printf() that takes a string like "Hello {0}!" where each {n} is replaced by the arg at that index.
 * Extra { and } characters may be stripped. We try to print out any invalid {n} indices but there is not a ton
 * of error-reporting. Try to pass valid input!
 */
String myprintf(String format, std::initializer_list<String> args, bool zeroTerminate=false)
{
	String result;

	StringBuilder stb = newStringBuilder(format.length * 2);

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
						if (parsedIndex >= 0 && parsedIndex < args.size())
						{
							succeeded = true;
							append(&stb, args.begin()[parsedIndex]);
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
		// We don't want the null termination byte to be included in the length, or else we get problems if
		// we myprintf() the result! Yes, this has happened. It was confusing.
		// - Sam, 10/12/2018
		stb.length--;
	}

	result = getString(&stb);

	return result;
}

inline String myprintf(char *format, std::initializer_list<String> args, bool zeroTerminate=false) { return myprintf(stringFromChars(format), args, zeroTerminate); }

const char* const intBaseChars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
String formatInt(u64 value, u8 base=10)
{
	ASSERT((base > 1) && (base <= 36), "formatInt() only handles base 2 to base 36.");
	char *temp = PushArray(globalFrameTempArena, char, 64); // Worst case is base 1, which is 64 characters!
	u32 count = 0;

	u64 v = value;

	do
	{
		temp[count++] = intBaseChars[v % base];
		v = v / base;
	}
	while (v != 0);

	// reverse it
	reverseString(temp, count);

	return makeString(temp, count);
}
inline String formatInt(u32 value, u8 base=10) {return formatInt((u64)value, base);}
inline String formatInt(u16 value, u8 base=10) {return formatInt((u64)value, base);}
inline String formatInt(u8  value, u8 base=10) {return formatInt((u64)value, base);}

String formatInt(s64 value, u8 base=10)
{
	ASSERT((base > 1) && (base <= 36), "formatInt() only handles base 2 to base 36.");
	char *temp = PushArray(globalFrameTempArena, char, 65); // Worst case is base 1, which is 64 characters! Plus 1 for sign
	bool isNegative = (value < 0);
	u32 count = 0;

	// One complication here: If we're passed s64_MIN, then -value is 1 larger than can be help in an s64!
	// So, rather than flipping it and treating it like a positive number with an '-' appended,
	// we have to make each digit positive as we get it.

	// s64 v = isNegative ? -value : value;
	s64 v = value;

	do
	{
		temp[count++] = intBaseChars[ ((isNegative ? -1 : 1) * (v % base)) ];
		v = v / base;
	}
	while (v != 0);

	if (isNegative)
	{
		temp[count++] = '-';
	}

	// reverse it
	reverseString(temp, count);

	return makeString(temp, count);
}
inline String formatInt(s32 value, u8 base=10) {return formatInt((s64)value, base);}
inline String formatInt(s16 value, u8 base=10) {return formatInt((s64)value, base);}
inline String formatInt(s8  value, u8 base=10) {return formatInt((s64)value, base);}

// TODO: Maybe do this properly ourselves rather than calling printf() internally? It's a bit janky.
String formatFloat(f64 value, s32 decimalPlaces)
{
	String formatString = myprintf("%.{0}f\0", {formatInt(decimalPlaces)});

	s32 length = 100; // TODO: is 100 enough?
	char *buffer = PushArray(globalFrameTempArena, char, length);
	s32 written = snprintf(buffer, length, formatString.chars, value);

	return makeString(buffer, MIN(written, length));
}

String formatString(String value, s32 length=-1, bool alignLeft = true, char paddingChar = ' ')
{
	if ((value.length == length) || (length == -1)) return value;

	if (length < value.length)
	{
		String result = makeString(value.chars, length);
		return result;
	}
	else
	{
		String result = newString(globalFrameTempArena, length);
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
String formatString(char *value, s32 length=-1, bool alignLeft = true, char paddingChar = ' ')
{
	return formatString(stringFromChars(value), length, alignLeft, paddingChar);
}

String formatBool(bool value)
{
	if (value) return stringFromChars("true");
	else       return stringFromChars("false");
}


String repeatChar(char c, s32 length)
{
	String result = newString(globalFrameTempArena, length);
	result.length = length;

	for (s32 i=0; i<length; i++)
	{
		result.chars[i] = c;
	}

	return result;
}