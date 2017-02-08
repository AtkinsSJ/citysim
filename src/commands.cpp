#pragma once
#pragma warning(push)
#pragma warning(disable: 4100) // Disable unused-arg warnings for commands, as they all have to take the same args.

struct Command
{
	String name;
	void (*function)(DebugConsole *console, TokenList *tokens);
};
Command *getCommand(int i);

#define ConsoleCommand(name) void cmd_##name(DebugConsole *console, TokenList *tokens)

ConsoleCommand(help)
{
	append(debugConsoleNextOutputLine(console), "Available commands are:");
	for (int i=0; getCommand(i); i++)
	{
		Command *cmd = getCommand(i);
		StringBuffer *line = debugConsoleNextOutputLine(console);
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
			StringBuffer *output = debugConsoleNextOutputLine(console);
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
		StringBuffer *output = debugConsoleNextOutputLine(console);
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

void debugHandleConsoleInput(DebugConsole *console)
{
	// copy input to output, for readability
	{
		StringBuffer *output = debugConsoleNextOutputLine(console);
		append(output, "> ");
		append(output, &console->input);
	}

	if (console->input.bufferLength != 0)
	{
		TokenList tokens = tokenize(bufferToString(&console->input));
		if (tokens.count > 0)
		{
			bool foundCommand = false;
			String firstToken = tokens.tokens[0];
			for (int i=0; i < ArrayCount(debugCommands); i++)
			{
				Command *cmd = debugCommands + i;
				if (equals(cmd->name, firstToken))
				{
					foundCommand = true;
					cmd->function(console, &tokens);
					break;
				}
			}

			if (!foundCommand)
			{
				StringBuffer *output = debugConsoleNextOutputLine(console);
				append(output, "I don't understand '");
				append(output, firstToken);
				append(output, "'. Try 'help' for a list of commands.");
			}
		}
	}

	// Do this last so we can actually read the input. To do otherwise would be Very Dumb™.
	clear(&console->input);
}

#pragma warning(pop)