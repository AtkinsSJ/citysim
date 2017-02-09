#pragma once

struct ConsoleTextState
{
	V2 pos;
	real32 maxWidth;

	UIState *uiState;
	RenderBuffer *uiBuffer;
};
inline ConsoleTextState initConsoleTextState(UIState *uiState, RenderBuffer *uiBuffer, V2 screenSize, real32 screenEdgePadding, real32 height)
{
	ConsoleTextState textState = {};
	textState.pos = v2(screenEdgePadding, height - screenEdgePadding);
	textState.maxWidth = screenSize.x - (2*screenEdgePadding);

	textState.uiState = uiState;
	textState.uiBuffer = uiBuffer;

	return textState;
}
void consoleTextOut(ConsoleTextState *textState, char *text, BitmapFont *font, ConsoleLineStyle style)
{
	int32 align = ALIGN_LEFT | ALIGN_BOTTOM;

	RealRect resultRect = uiText(textState->uiState, textState->uiBuffer, font, text, textState->pos,
	                             align, 300, style.textColor, textState->maxWidth);
	textState->pos.y -= resultRect.h;
}

void renderConsole(Console *console, UIState *uiState, RenderBuffer *uiBuffer)
{
	drawRect(uiBuffer, rectXYWH(0,0,uiBuffer->camera.size.x, console->height), 100, color255(0,0,0,245));
	ConsoleTextState textState = initConsoleTextState(uiState, uiBuffer, uiBuffer->camera.size, 8.0f, console->height);

	// Caret stuff is a bit hacky, but oh well.
	// It especially doesn't play well with pausing the debug capturing...
	char caret = '_';
	if (console->caretFlashCounter < 0.5f)
	{
		caret = ' ';
	}
	char buffer[consoleLineLength + 3];
	snprintf(buffer, sizeof(buffer), "> %.*s%c", console->input.bufferLength, console->input.buffer, caret);
	consoleTextOut(&textState, buffer, console->font, console->styles[CLS_Input]);

	// print output lines
	for (int32 i=console->outputLineCount-1; i>=0; i--)
	{
		ConsoleOutputLine *line = console->outputLines + WRAP(console->currentOutputLine + i, console->outputLineCount);
		consoleTextOut(&textState, line->buffer.buffer, console->font, console->styles[line->style]);

		// If we've gone off the screen, stop!
		if ((textState.pos.y < 0) || (textState.pos.y > uiBuffer->camera.size.y))
		{
			break;
		}
	}
	

	console->caretFlashCounter = fmod(console->caretFlashCounter + SECONDS_PER_FRAME, 1.0f);
}

void consoleHandleCommand(Console *console)
{
	// copy input to output, for readability
	{
		StringBuffer *output = consoleNextOutputLine(console, CLS_InputEcho);
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
				StringBuffer *output = consoleNextOutputLine(console, CLS_Error);
				append(output, "I don't understand '");
				append(output, firstToken);
				append(output, "'. Try 'help' for a list of commands.");
			}
		}
	}

	// Do this last so we can actually read the input. To do otherwise would be Very Dumb™.
	clear(&console->input);
}

void updateConsole(Console *console, InputState *inputState, UIState *uiState, RenderBuffer *uiBuffer)
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
		renderConsole(console, uiState, uiBuffer);
	}
}
