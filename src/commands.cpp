#pragma once
#pragma warning(push)
#pragma warning(disable: 4100) // Disable unused-arg warnings for commands, as they all have to take the same args.

struct Command
{
	String name;
	void (*function)(Console *console, TokenList *tokens);
};
Command *getCommand(int i);

#define ConsoleCommand(name) void cmd_##name(Console *console, TokenList *tokens)

ConsoleCommand(help)
{
	append(consoleNextOutputLine(console), "Available commands are:");
	for (int i=0; getCommand(i); i++)
	{
		Command *cmd = getCommand(i);
		StringBuffer *line = consoleNextOutputLine(console);
		append(line, "    ");
		append(line, cmd->name);
	}
}

ConsoleCommand(resize_window)
{
	bool succeeded = false;

	if (tokens->count == 3)
	{
		String sWidth = tokens->tokens[1];
		String sHeight = tokens->tokens[2];
		
		int32 width = 0;
		int32 height = 0;
		if (asInt(sWidth, &width)   && (width > 0)
		 && asInt(sHeight, &height) && (height > 0))
		{
			StringBuffer *output = consoleNextOutputLine(console);
			append(output, "Resizing window to ");
			append(output, sWidth);
			append(output, " by ");
			append(output, sHeight);
			succeeded = true;
			resizeWindow(globalDebugState->renderer, width, height);
		}
	}

	if (!succeeded)
	{
		StringBuffer *output = consoleNextOutputLine(console);
		append(output, "Usage: ");
		append(output, tokens->tokens[0]);
		append(output, " width height");
	}
}

Command debugCommands[] = {
	{stringFromChars("help"), &cmd_help},
	{stringFromChars("resize_window"), &cmd_resize_window},
};
inline Command *getCommand(int i) {
	if ((i < 0) || (i >= ArrayCount(debugCommands))) return null;

	return debugCommands + i;
}

#pragma warning(pop)