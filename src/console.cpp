#pragma once

static Console theConsole;

void consoleWriteLine(String text, ConsoleLineStyleID style)
{
	if (globalConsole)
	{
		ConsoleOutputLine *line = appendBlank(&globalConsole->outputLines);
		line->style = style;
		line->text = pushString(globalConsole->outputLines.memoryArena, text);
	}
}

void initConsole(MemoryArena *debugArena, f32 openHeight, f32 maximisedHeight, f32 openSpeed)
{
	Console *console = &theConsole;
	console->currentHeight = 0;

	console->openHeight = openHeight;
	console->maximisedHeight = maximisedHeight;
	console->openSpeed = openSpeed;

	console->input = newTextInput(debugArena, consoleLineLength);
	initChunkedArray(&console->inputHistory, debugArena, 256);
	console->inputHistoryCursor = -1;

	initChunkedArray(&console->outputLines, debugArena, 1024);
	console->scrollPos = 0;

	initChunkedArray(&console->commands, &globalAppState.systemArena, 64);
	initChunkedArray(&console->commandShortcuts, &globalAppState.systemArena, 64);

	initCommands(console);
	consoleWriteLine(myprintf("Loaded {0} commands. Type 'help' to list them."_s, {formatInt(console->commands.count)}), CLS_Default);

	globalConsole = console;

	consoleWriteLine("GREETINGS PROFESSOR FALKEN.\nWOULD YOU LIKE TO PLAY A GAME?"_s);
}

void renderConsole(Console *console)
{
	RenderBuffer *renderBuffer = &renderer->debugBuffer;

	s32 actualConsoleHeight = floor_s32(console->currentHeight * renderer->uiCamera.size.y);

	s32 screenWidth = round_s32(renderer->uiCamera.size.x);

	UIConsoleStyle *consoleStyle = findConsoleStyle(&assets->theme, "default"_s);
	UITextInputStyle *textInputStyle = findTextInputStyle(&assets->theme, consoleStyle->textInputStyleName);
	BitmapFont *consoleFont = getFont(consoleStyle->fontName);

	V2I textPos = v2i(consoleStyle->padding, actualConsoleHeight - consoleStyle->padding);
	s32 textMaxWidth = screenWidth - (2*consoleStyle->padding);

	RenderItem_DrawSingleRect *consoleBackground = appendDrawRectPlaceholder(renderBuffer, renderer->shaderIds.untextured);
	RenderItem_DrawSingleRect *inputBackground   = appendDrawRectPlaceholder(renderBuffer, renderer->shaderIds.untextured);

	Rect2I textInputRect = drawTextInput(renderBuffer, &console->input, textInputStyle, textPos, ALIGN_LEFT | ALIGN_BOTTOM, textMaxWidth);
	textPos.y -= textInputRect.h;

	textPos.y -= consoleStyle->padding;

	s32 heightOfOutputArea = textPos.y;

	// draw backgrounds now we know size of input area
	// TODO: Draw the input background inside drawTextInput()!
	Rect2I inputBackRect = irectXYWH(0, textPos.y, screenWidth, actualConsoleHeight - heightOfOutputArea);
	fillDrawRectPlaceholder(inputBackground, inputBackRect, textInputStyle->backgroundColor);
	Rect2I consoleBackRect = irectXYWH(0,0,screenWidth, heightOfOutputArea);
	fillDrawRectPlaceholder(consoleBackground, consoleBackRect, consoleStyle->backgroundColor);

	V2I knobSize = v2i(consoleStyle->scrollBarWidth, consoleStyle->scrollBarWidth);
	f32 scrollPercent = 1.0f - ((f32)console->scrollPos / (f32)consoleMaxScrollPos(console));
	drawScrollBar(renderBuffer, v2i(screenWidth - knobSize.x, 0), heightOfOutputArea, scrollPercent, knobSize, consoleStyle->scrollBarKnobColor, renderer->shaderIds.untextured);

	textPos.y -= consoleStyle->padding;

	// print output lines
	s32 outputLinesAlign = ALIGN_LEFT | ALIGN_BOTTOM;
	for (auto it = iterate(&console->outputLines, console->outputLines.count - console->scrollPos - 1, false, true);
		hasNext(&it);
		next(&it))
	{
		ConsoleOutputLine *line = get(&it);
		V4 outputTextColor = consoleStyle->outputTextColor[line->style];

		Rect2I resultRect = uiText(renderBuffer, consoleFont, line->text, textPos, outputLinesAlign, outputTextColor, textMaxWidth);
		textPos.y -= resultRect.h;

		// If we've gone off the screen, stop!
		if ((textPos.y < 0) || (textPos.y > renderer->uiCamera.size.y))
		{
			break;
		}
	}
}

void consoleHandleCommand(Console *console, String commandInput)
{
	// copy input to output, for readability
	consoleWriteLine(myprintf("> {0}"_s, {commandInput}), CLS_InputEcho);

	if (!isEmpty(commandInput))
	{
		// Add to history
		append(&console->inputHistory, pushString(console->inputHistory.memoryArena, commandInput));
		console->inputHistoryCursor = -1;

		s32 tokenCount = countTokens(commandInput);
		if (tokenCount > 0)
		{
			bool foundCommand = false;
			String arguments;
			String firstToken = nextToken(commandInput, &arguments);

			for (auto it = iterate(&globalConsole->commands);
				hasNext(&it);
				next(&it))
			{
				Command *command = get(&it);

				if (equals(command->name, firstToken))
				{
					foundCommand = true;

					s32 argCount = tokenCount - 1;
					bool tooManyArgs = (argCount > command->maxArgs) && (command->maxArgs != -1);
					if ((argCount < command->minArgs) || tooManyArgs)
					{
						if (command->minArgs == command->maxArgs)
						{
							consoleWriteLine(myprintf("Command '{0}' accepts only {1} argument(s), but {2} given."_s,
								{firstToken, formatInt(command->minArgs), formatInt(argCount)}), CLS_Error);
						}
						else
						{
							consoleWriteLine(myprintf("Command '{0}' accepts between {1} and {2} arguments, but {3} given."_s,
								{firstToken, formatInt(command->minArgs), formatInt(command->maxArgs), formatInt(argCount)}), CLS_Error);
						}
					}
					else
					{
						u32 commandStartTime = SDL_GetTicks();
						command->function(console, argCount, arguments);
						u32 commandEndTime = SDL_GetTicks();

						consoleWriteLine(myprintf("Command executed in {0}ms"_s, {formatInt(commandEndTime - commandStartTime)}));
					}

					break;
				}
			}

			if (!foundCommand)
			{
				consoleWriteLine(myprintf("I don't understand '{0}'. Try 'help' for a list of commands."_s, {firstToken}), CLS_Error);
			}
		}
	}
}

void updateConsole(Console *console)
{
	// Keyboard shortcuts for commands
	for (auto it = iterate(&console->commandShortcuts);
		hasNext(&it);
		next(&it))
	{
		CommandShortcut *shortcut = get(&it);

		if (wasShortcutJustPressed(shortcut->shortcut))
		{
			consoleHandleCommand(console, shortcut->command);
			console->scrollPos = 0;
		}
	}

	// Show/hide the console
	if (keyJustPressed(SDLK_TAB))
	{
		if (modifierKeyIsPressed(KeyMod_Ctrl))
		{
			if (console->targetHeight == console->maximisedHeight)
			{
				console->targetHeight = console->openHeight;
			}
			else
			{
				console->targetHeight = console->maximisedHeight;
			}
			console->scrollPos = 0;
		}
		else
		{
			if (console->targetHeight > 0)
			{
				console->targetHeight = 0;
			}
			else
			{
				console->targetHeight = console->openHeight;
				console->scrollPos = 0;
			}
		}
	}

	if (console->currentHeight != console->targetHeight)
	{
		console->currentHeight = approach(console->currentHeight, console->targetHeight, console->openSpeed * SECONDS_PER_FRAME);
	}

	if (console->currentHeight > 0)
	{
		if (updateTextInput(&console->input))
		{
			consoleHandleCommand(console, textInputToString(&console->input));
			console->scrollPos = 0;
			clear(&console->input);
		}
		else
		{
			// Command history
			if (console->inputHistory.count > 0)
			{
				if (keyJustPressed(SDLK_UP))
				{
					if (console->inputHistoryCursor == -1)
					{
						console->inputHistoryCursor = truncate32(console->inputHistory.count - 1);
					}
					else
					{
						console->inputHistoryCursor = max(console->inputHistoryCursor - 1, 0);
					}

					clear(&console->input);
					String oldInput = *get(&console->inputHistory, console->inputHistoryCursor);
					append(&console->input, oldInput);
				}
				else if (keyJustPressed(SDLK_DOWN))
				{
					if (console->inputHistoryCursor == -1)
					{
						console->inputHistoryCursor = truncate32(console->inputHistory.count - 1);
					}
					else
					{
						console->inputHistoryCursor = truncate32(min(console->inputHistoryCursor + 1, (s32)console->inputHistory.count - 1));
					}

					clear(&console->input);
					String oldInput = *get(&console->inputHistory, console->inputHistoryCursor);
					append(&console->input, oldInput);
				}
			}
		}

		// scrolling!
		if (inputState->wheelY != 0)
		{
			console->scrollPos = clamp(console->scrollPos + (inputState->wheelY * 3), 0, consoleMaxScrollPos(console));
		}
	}
}

void loadConsoleKeyboardShortcuts(Console *console, Blob data, String filename)
{
	LineReader reader = readLines(filename, data);

	clear(&console->commandShortcuts);

	while (loadNextLine(&reader))
	{
		String shortcutString = readToken(&reader);
		String command = getRemainderOfLine(&reader);

		KeyboardShortcut shortcut = parseKeyboardShortcut(shortcutString);
		if (shortcut.key == SDLK_UNKNOWN)
		{
			error(&reader, "Unrecognised key in keyboard shortcut sequence '{0}'"_s, {shortcutString});
		}
		else
		{
			CommandShortcut *commandShortcut = appendBlank(&console->commandShortcuts);
			commandShortcut->shortcut = shortcut;
			commandShortcut->command = command;
		}
	}
}
