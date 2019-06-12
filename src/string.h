#pragma once

struct String
{
	s32 length;
	s32 maxLength; // TODO: @Size Maybe we should have a separate struct for editable Strings? That way String itself can be small, PLUS we'd know that the hash value was correct.
	char *chars;

	bool hasHash;
	u32 hash;
};

const String nullString = {};

u32 hashString(String *s);

inline String makeString(char *chars, s32 length, bool hash=false)
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
inline String makeString(char *chars, bool hash=false)
{
	return makeString(chars, truncate32(strlen(chars)), hash);
}
// const is a huge pain in the bum
inline String makeString(const char *chars, bool hash=false)
{
	return makeString((char*)chars, truncate32(strlen(chars)), hash);
}

inline String stringFromBlob(Blob blob, bool hash=false)
{
	return makeString((char*)blob.memory, truncate32(blob.size), hash);
}

void copyString(char *src, s32 srcLength, String *dest);
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
	s32 len = truncate32(strlen(src));

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

bool equals(String a, String b);
inline bool equals(String a, char *b)
{
	return equals(a, makeString(b));
}

// Like strcmp()
s32 compare(String a, String b);

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

inline bool isNullTerminated(String s)
{
	// A 0-length string, by definition, can't have a null terminator
	bool result = (s.length > 0) && (s.chars[s.length-1] == 0);
	return result;
}

String formatInt(u64 value, u8 base=10);
inline String formatInt(u32 value, u8 base=10) {return formatInt((u64)value, base);}
inline String formatInt(u16 value, u8 base=10) {return formatInt((u64)value, base);}
inline String formatInt(u8  value, u8 base=10) {return formatInt((u64)value, base);}

String formatInt(s64 value, u8 base=10);
inline String formatInt(s32 value, u8 base=10) {return formatInt((s64)value, base);}
inline String formatInt(s16 value, u8 base=10) {return formatInt((s64)value, base);}
inline String formatInt(s8  value, u8 base=10) {return formatInt((s64)value, base);}

String formatFloat(f64 value, s32 decimalPlaces);
inline String formatFloat(f32 value, s32 decimalPlaces) {return formatFloat((f64)value, decimalPlaces);}

String formatString(String value, s32 length=-1, bool alignLeft = true, char paddingChar = ' ');
inline String formatString(char *value, s32 length=-1, bool alignLeft = true, char paddingChar = ' ')
{
	return formatString(makeString(value), length, alignLeft, paddingChar);
}

String formatBool(bool value);

String myprintf(String format, std::initializer_list<String> args, bool zeroTerminate=false);
inline String myprintf(char *format, std::initializer_list<String> args, bool zeroTerminate=false) { return myprintf(makeString(format), args, zeroTerminate); }
