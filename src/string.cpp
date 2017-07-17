#pragma once
#include <initializer_list>

/**
 * A printf() that takes a string like "Hello {0}!" where each {n} is replaced by the arg at that index.
 * Extra { and } characters may be stripped. We try to print out any invalid {n} indices but there is not a ton
 * of error-reporting. Try to pass valid input!
 */
String myprintf(String format, std::initializer_list<String> args)
{
	String result;

	StringBuffer stb = newStringBuffer(globalFrameTempArena, format.length * 4);

	int32 startOfNumber = INT32_MAX;
	bool isReadingNumber = false;
	for (int32 i=0; i<format.length; i++)
	{
		switch (format.chars[i])
		{
			case '{': {
				startOfNumber = i+1;
				isReadingNumber = true;
			} break;

			case '}': {
				int32 endOfNumber = i;
				bool succeeded = false;
				if (isReadingNumber && endOfNumber > startOfNumber)
				{
					String indexString = makeString(format.chars + startOfNumber, (endOfNumber - startOfNumber));
					int64 parsedIndex = 0;
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
				startOfNumber = INT32_MAX;
			} break;

			default: {
				if (!isReadingNumber)
				{
					append(&stb, format.chars + i, 1);
				}
			}
		}
	}

	result = bufferToString(&stb);

	return result;
}

inline String myprintf(char *format, std::initializer_list<String> args) { return myprintf(stringFromChars(format), args); }

String formatInt(int64 value)
{
	char *temp = PushArray(globalFrameTempArena, char, 20); // Largest 64 bit signed value is 19 characters long, plus a '-', so 20 again. Yay!
	uint32 count = 0;
	bool isNegative = (value < 0);

	// One complication here: If we're passed INT64_MIN, then -value is 1 larger than can be help in an INT64!
	// So, rather than flipping it and treating it like a positive number with an '-' appended,
	// we have to make each digit positive as we get it.

	// int64 v = isNegative ? -value : value;
	int64 v = value;

	do
	{
		temp[count++] = '0' + ((isNegative ? -1 : 1) * (v % 10));
		v = v / 10;
	}
	while (v != 0);

	if (isNegative)
	{
		temp[count++] = '-';
	}

	// reverse it
	reverseString(temp, count);

	// append the chars
	return makeString(temp, count);
}