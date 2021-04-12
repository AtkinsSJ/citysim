#pragma once

//
// NB: Strings are UTF-8. The length and maxLength are measured in bytes, not in glyphs.
// Probably one day we should make this consistent with TextInput using both, but eh.
//

struct String
{
	s32 length;
	s32 maxLength; // TODO: @Size Maybe we should have a separate struct for editable Strings? That way String itself can be small, PLUS we'd know that the hash value was correct.
	char *chars;

	bool hasHash;
	u32 hash;

	char &operator[](s32 index);

	bool isNullTerminated();
	bool startsWith(String prefix);
	bool endsWith(String suffix);
	bool isEmpty();
};

const String nullString = {};

String makeString(char *chars, s32 length, bool hash=false);
String makeString(char *chars, bool hash=false);
String makeString(const char *chars, bool hash=false);
String stringFromBlob(Blob blob, bool hash=false);
String repeatChar(char c, s32 length);

inline String operator"" _s(const char *chars, size_t length)
{
	return makeString((char*)chars, (s32)length);
}

inline String operator"" _h(const char *chars, size_t length)
{
	return makeString((char*)chars, (s32)length, true);
}

void copyString(char *src, s32 srcLength, String *dest);
void copyString(String src, String *dest);

String pushString(MemoryArena *arena, s32 length, bool setLength=false);
String pushString(MemoryArena *arena, char *src);
String pushString(MemoryArena *arena, String src);

u32 hashString(String *s);

bool equals(String a, String b);
bool stringIsValid(String s);

void reverse(char* first, u32 length);
String trim(String input);
String trimStart(String input);
String trimEnd(String input);

Maybe<s64> asInt(String input);
Maybe<f64> asFloat(String input);
Maybe<bool> asBool(String input);

bool isNullTerminated(String s);
bool isEmpty(String s);

Maybe<s32> findIndexOfChar(String input, char c, bool searchFromEnd, s32 startIndex=-1);
bool contains(String input, char c);

// TODO: Add "splitchar" support to countTokens() as well
s32 countTokens(String input);
// If splitChar is provided, the token ends before that, and it is skipped.
// Otherwise, we stop at the first whitespace character, determined by isWhitespace()
String nextToken(String input, String *remainder, char splitChar = 0);
// NB: You can pass null for leftResult or rightResult to ignore that part.
bool splitInTwo(String input, char divider, String *leftResult, String *rightResult);

String concatenate(std::initializer_list<String> strings, String between=nullString);

String formatInt(u64 value, u8 base=10, s32 zeroPadWidth=0);
inline String formatInt(u32 value, u8 base=10, s32 zeroPadWidth=0) {return formatInt((u64)value, base, zeroPadWidth);}
inline String formatInt(u16 value, u8 base=10, s32 zeroPadWidth=0) {return formatInt((u64)value, base, zeroPadWidth);}
inline String formatInt(u8  value, u8 base=10, s32 zeroPadWidth=0) {return formatInt((u64)value, base, zeroPadWidth);}

String formatInt(s64 value, u8 base=10, s32 zeroPadWidth=0);
inline String formatInt(s32 value, u8 base=10, s32 zeroPadWidth=0) {return formatInt((s64)value, base, zeroPadWidth);}
inline String formatInt(s16 value, u8 base=10, s32 zeroPadWidth=0) {return formatInt((s64)value, base, zeroPadWidth);}
inline String formatInt(s8  value, u8 base=10, s32 zeroPadWidth=0) {return formatInt((s64)value, base, zeroPadWidth);}

String formatFloat(f64 value, s32 decimalPlaces);
inline String formatFloat(f32 value, s32 decimalPlaces) {return formatFloat((f64)value, decimalPlaces);}

String formatString(String value, s32 length=-1, bool alignLeft = true, char paddingChar = ' ');
inline String formatString(char *value, s32 length=-1, bool alignLeft = true, char paddingChar = ' ')
{
	return formatString(makeString(value), length, alignLeft, paddingChar);
}

String formatBool(bool value);

String myprintf(String format, std::initializer_list<String> args, bool zeroTerminate=false);
