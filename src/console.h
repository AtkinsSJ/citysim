#pragma once

struct DebugConsole
{
	bool isVisible;
	struct BitmapFont *font;

	StringBuffer input;
	int32 outputLineCount;
	StringBuffer *outputLines;
	int32 currentOutputLine;
};

void initDebugConsole(MemoryArena *debugArena, DebugConsole *console, int32 lineLength, int32 outputLineCount,
	                  BitmapFont *font)
{
	console->isVisible = true;
	console->font = font;

	console->input = newStringBuffer(debugArena, lineLength);
	console->outputLineCount = outputLineCount;
	console->outputLines = PushArray(debugArena, StringBuffer, console->outputLineCount);
	for (int32 i=0; i < console->outputLineCount; i++)
	{
		console->outputLines[i] = newStringBuffer(debugArena, lineLength);
	}
}

StringBuffer *debugConsoleNextOutputLine(DebugConsole *console)
{
	StringBuffer *result = console->outputLines + console->currentOutputLine;
	console->currentOutputLine = WRAP(console->currentOutputLine + 1, console->outputLineCount);

	clear(result);

	return result;
}