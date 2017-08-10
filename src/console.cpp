#pragma once

static Console theConsole;

void consoleWriteLine(String text, ConsoleLineStyleID style)
{
	if (globalDebugState)
	{
		String *line = consoleNextOutputLine(globalConsole, style);
		line->length = MIN(text.length, line->maxLength);
		for (int32 i=0; i<line->length; i++)
		{
			line->chars[i] = text.chars[i];
		}
	}
}

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

RealRect consoleTextOut(ConsoleTextState *textState, String text, BitmapFont *font, ConsoleLineStyle style)
{
	int32 align = ALIGN_LEFT | ALIGN_BOTTOM;

	RealRect resultRect = uiText(textState->uiState, textState->uiBuffer, font, text, textState->pos,
	                             align, 300, style.textColor, textState->maxWidth);
	textState->pos.y -= resultRect.h;

	return resultRect;
}

void initConsole(MemoryArena *debugArena, int32 outputLineCount, BitmapFont *font, real32 height)
{
	Console *console = &theConsole;
	console->isVisible = false;
	console->font = font;
	console->styles[CLS_Default].textColor   = color255(192, 192, 192, 255);
	console->styles[CLS_InputEcho].textColor = color255(128, 128, 128, 255);
	console->styles[CLS_Error].textColor     = color255(255, 128, 128, 255);
	console->styles[CLS_Success].textColor   = color255(128, 255, 128, 255);
	console->styles[CLS_Input].textColor     = color255(255, 255, 255, 255);
	console->height = height;

	console->input = newTextInput(debugArena, consoleLineLength);
	console->charWidth = findChar(console->font, 'M')->xAdvance;

	console->outputLineCount = outputLineCount;
	console->outputLines = PushArray(debugArena, ConsoleOutputLine, console->outputLineCount);
	for (int32 i=0; i < console->outputLineCount; i++)
	{
		console->outputLines[i].text = newString(debugArena, consoleLineLength);
		console->outputLines[i].style = CLS_Default;
	}

	globalConsole = console;

	// temp test stuff goes here
	// consoleWriteLine(myprintf("}{-1} {5} {{}{pineapple}!", {stringFromChars("Hello"), stringFromChars("World")}));
	consoleWriteLine("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
	consoleWriteLine("GREETINGS PROFESSOR FALKEN.\nWOULD YOU LIKE TO PLAY A GAME?");
	consoleWriteLine(myprintf("My favourite number is {0}!", {formatFloat(12345.6789, 3)}));
}

void renderConsole(Console *console, UIState *uiState, RenderBuffer *uiBuffer)
{
	ConsoleTextState textState = initConsoleTextState(uiState, uiBuffer, uiBuffer->camera.size, 8.0f, console->height);

	bool showCaret = (console->caretFlashCounter < 0.5f);
	RealRect textInputRect = drawTextInput(uiState, uiBuffer, console->font, &console->input, showCaret, textState.pos, ALIGN_LEFT | ALIGN_BOTTOM, 300, console->styles[CLS_Input].textColor, textState.maxWidth);
	textState.pos.y -= textInputRect.h;

	// consoleTextOut(&textState, makeString(console->input.buffer, console->input.byteLength), console->font, console->styles[CLS_Input]);

	// if (showCaret)
	// {
	// 	// Not quite correct. It assumes all lines are full, whereas our line-wrapping happens at
	// 	// word boundaries so the line can be shorter than charsPerLine.
	// 	int32 charsPerLine = (int32) (textState.maxWidth / console->charWidth);
	// 	int32 caretX = console->input.caretGlyphPos % charsPerLine;
	// 	int32 caretY = console->input.caretGlyphPos / charsPerLine;

	// 	drawRect(uiBuffer,
	// 		     rectXYWH(textState.pos.x + (caretX * console->charWidth), textState.pos.y + (caretY * console->font->lineHeight), 2, console->font->lineHeight),
	// 	         310, console->styles[CLS_Input].textColor);
	// }

	textState.pos.y -= 8.0f;

	// draw backgrounds now we know size of input area
	drawRect(uiBuffer, rectXYWH(0,textState.pos.y,uiBuffer->camera.size.x, console->height - textState.pos.y), 100, color255(64,64,64,245));
	drawRect(uiBuffer, rectXYWH(0,0,uiBuffer->camera.size.x, textState.pos.y), 100, color255(0,0,0,245));

	textState.pos.y -= 8.0f;

	// print output lines
	for (int32 i=console->outputLineCount-1; i>=0; i--)
	{
		ConsoleOutputLine *line = console->outputLines + WRAP(console->currentOutputLine + i, console->outputLineCount);
		consoleTextOut(&textState, line->text, console->font, console->styles[line->style]);

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
	String inputS = textInputToString(&console->input);
	consoleWriteLine(myprintf("> {0}", {inputS}), CLS_InputEcho);

	if (console->input.glyphLength != 0)
	{
		TokenList tokens = tokenize(inputS);
		if (tokens.count > 0)
		{
			bool foundCommand = false;
			String firstToken = tokens.tokens[0];
			for (int i=0; i < consoleCommands.count; i++)
			{
				Command cmd = consoleCommands[i];
				if (equals(cmd.name, firstToken))
				{
					foundCommand = true;

					int32 argCount = tokens.count-1;
					if ((argCount < cmd.minArgs) || (argCount > cmd.maxArgs))
					{
						if (cmd.minArgs == cmd.maxArgs)
						{
							consoleWriteLine(myprintf("Command '{0}' accepts only {1} argument(s), but {2} given.",
								{firstToken, formatInt(cmd.minArgs), formatInt(argCount)}), CLS_Error);
						}
						else
						{
							consoleWriteLine(myprintf("Command '{0}' accepts between {1} and {2} arguments, but {3} given.",
								{firstToken, formatInt(cmd.minArgs), formatInt(cmd.maxArgs), formatInt(argCount)}), CLS_Error);
						}
					}
					else
					{
						uint32 commandStartTime = SDL_GetTicks();
						cmd.function(console, &tokens);
						uint32 commandEndTime = SDL_GetTicks();

						consoleWriteLine(myprintf("Command executed in {0}ms", {formatInt(commandEndTime - commandStartTime)}));
					}

					break;
				}
			}

			if (!foundCommand)
			{
				consoleWriteLine(myprintf("I don't understand '{0}'. Try 'help' for a list of commands.", {firstToken}), CLS_Error);
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
			console->caretFlashCounter = 0;
		}
		else if (keyJustPressed(inputState, SDLK_BACKSPACE))
		{
			backspace(&console->input);
			console->caretFlashCounter = 0;
		}
		if (keyJustPressed(inputState, SDLK_DELETE))
		{
			deleteChar(&console->input);
			console->caretFlashCounter = 0;
		}

		if (keyJustPressed(inputState, SDLK_RETURN))
		{
			consoleHandleCommand(console);
			console->caretFlashCounter = 0;
		}

		if (keyJustPressed(inputState, SDLK_LEFT))
		{
			moveCaretLeft(&console->input, 1);
			console->caretFlashCounter = 0;
		}
		else if (keyJustPressed(inputState, SDLK_RIGHT))
		{
			moveCaretRight(&console->input, 1);
			console->caretFlashCounter = 0;
		}
		if (keyJustPressed(inputState, SDLK_HOME))
		{
			console->input.caretBytePos = 0;
			console->input.caretGlyphPos = 0;
			console->caretFlashCounter = 0;
		}
		if (keyJustPressed(inputState, SDLK_END))
		{
			console->input.caretBytePos = console->input.byteLength;
			console->input.caretGlyphPos = console->input.glyphLength;
			console->caretFlashCounter = 0;
		}

		if (wasTextEntered(inputState))
		{
			char *enteredText = getEnteredText(inputState);
			int32 inputTextLength = strlen(enteredText);

			insert(&console->input, enteredText, inputTextLength);
			console->caretFlashCounter = 0;
		}

		if (keyJustPressed(inputState, SDLK_v, KeyMod_Ctrl))
		{
			if (SDL_HasClipboardText())
			{
				char *clipboard = SDL_GetClipboardText();
				if (clipboard)
				{
					int32 clipboardLength = strlen(clipboard);
					insert(&console->input, clipboard, clipboardLength);
					console->caretFlashCounter = 0;

					SDL_free(clipboard);
				}
			}
		}

		renderConsole(console, uiState, uiBuffer);
	}
}
