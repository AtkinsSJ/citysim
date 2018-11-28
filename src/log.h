#pragma once

struct String;

void log(char *format, std::initializer_list<String> args = {});

#define LOGFUNCDEF(TYPE) void log##TYPE(char *format, std::initializer_list<String> args = {});

LOGFUNCDEF(Verbose);
LOGFUNCDEF(Debug);
LOGFUNCDEF(Info);
LOGFUNCDEF(Warn);
LOGFUNCDEF(Error);
LOGFUNCDEF(Critical);