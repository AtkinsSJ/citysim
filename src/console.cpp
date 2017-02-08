#pragma once

void renderDebugConsole(DebugConsole *console, UIState *uiState, RenderBuffer *uiBuffer)
{
	DebugTextState textState = initDebugTextState(uiState, uiBuffer, console->font, makeWhite(),
		                                          uiBuffer->camera.size, 16.0f, true);

	// Caret stuff is a bit hacky, but oh well.
	// It especially doesn't play well with pausing the debug capturing...
	char caret = '_';
	// if ((debugState->readingFrameIndex % FRAMES_PER_SECOND) < (FRAMES_PER_SECOND/2))
	// {
	// 	caret = ' ';
	// }
	debugTextOut(&textState, "> %.*s%c", console->input.bufferLength, console->input.buffer, caret);

	// print output lines
	for (int32 i=console->outputLineCount-1; i>=0; i--)
	{
		StringBuffer *line = console->outputLines + WRAP(console->currentOutputLine + i, console->outputLineCount);
		debugTextOut(&textState, "%s", line->buffer);
	}
	
	drawRect(uiBuffer, rectXYWH(0,0,uiBuffer->camera.size.x, uiBuffer->camera.size.y),
			 100, color255(0,0,0,128));
}

void consoleHandleCommand(DebugConsole *console)
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

void updateDebugConsole(DebugConsole *console, InputState *inputState, UIState *uiState, RenderBuffer *uiBuffer)
{
	if (keyJustPressed(inputState, SDLK_TAB))
	{
		console->isVisible = !console->isVisible;
	}

	if (console->isVisible)
	{
		if (keyJustPressed(inputState, SDLK_BACKSPACE, KeyMod_Ctrl))
		{
			clear(&console->input);
		}
		else if (keyJustPressed(inputState, SDLK_BACKSPACE))
		{
			backspace(&console->input);
		}
		else if (keyJustPressed(inputState, SDLK_RETURN))
		{
			consoleHandleCommand(console);
		}

		if (wasTextEntered(inputState))
		{
			char *enteredText = getEnteredText(inputState);
			int32 inputTextLength = strlen(enteredText);
			append(&console->input, enteredText, inputTextLength);
		}
		renderDebugConsole(console, uiState, uiBuffer);
	}
}
