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
	console->styles[CLS_Default].textColor   = color255(192, 192, 192, 255);
	console->styles[CLS_InputEcho].textColor = color255(128, 128, 128, 255);
	console->styles[CLS_Error].textColor     = color255(255, 128, 128, 255);
	console->styles[CLS_Warning].textColor   = color255(255, 255, 128, 255);
	console->styles[CLS_Success].textColor   = color255(128, 255, 128, 255);
	console->styles[CLS_Input].textColor     = color255(255, 255, 255, 255);

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
	consoleWriteLine(myprintf("Loaded {0} commands. Type 'help' to list them.", {formatInt(console->commands.count)}), CLS_Default);

	globalConsole = console;

	consoleWriteLine("GREETINGS PROFESSOR FALKEN.\nWOULD YOU LIKE TO PLAY A GAME?");
}

void renderConsole(Console *console, Renderer *renderer)
{
	BitmapFont *consoleFont = getFont(theAssets, makeString("debug"));
	RenderBuffer *renderBuffer = &renderer->debugBuffer;

	f32 actualConsoleHeight = console->currentHeight * renderer->uiCamera.size.y;

	f32 screenEdgePadding = 8.0f;
	V2 textPos = v2(screenEdgePadding, round_f32(actualConsoleHeight - screenEdgePadding));
	f32 textMaxWidth = renderer->uiCamera.size.x - (2*screenEdgePadding);

	RenderItem_DrawSingleRect *consoleBackground = appendDrawRectPlaceholder(renderBuffer, renderer->shaderIds.untextured);
	RenderItem_DrawSingleRect *inputBackground   = appendDrawRectPlaceholder(renderBuffer, renderer->shaderIds.untextured);

	Rect2 textInputRect = drawTextInput(renderer, renderBuffer, consoleFont, &console->input, textPos, ALIGN_LEFT | ALIGN_BOTTOM, console->styles[CLS_Input].textColor, textMaxWidth);
	textPos.y -= textInputRect.h;

	textPos.y -= 8.0f;

	// draw backgrounds now we know size of input area
	Rect2 inputBackRect = rectXYWH(0,textPos.y,renderer->uiCamera.size.x, actualConsoleHeight - textPos.y);
	fillDrawRectPlaceholder(inputBackground, inputBackRect, color255(64,64,64,245));
	Rect2 consoleBackRect = rectXYWH(0,0,renderer->uiCamera.size.x, textPos.y);
	fillDrawRectPlaceholder(consoleBackground, consoleBackRect, color255(0,0,0,245));

	V2 knobSize = v2(12.0f, 64.0f);
	f32 scrollPercent = 1.0f - ((f32)console->scrollPos / (f32)consoleMaxScrollPos(console));
	drawScrollBar(renderBuffer, v2(renderer->uiCamera.size.x - knobSize.x, 0.0f), consoleBackRect.h, scrollPercent, knobSize, color255(48, 48, 48, 245), renderer->shaderIds.untextured);

	textPos.y -= 8.0f;

	// print output lines
	s32 outputLinesAlign = ALIGN_LEFT | ALIGN_BOTTOM;
	for (auto it = iterate(&console->outputLines, console->outputLines.count - console->scrollPos - 1, false, true);
		!it.isDone;
		next(&it))
	{
		ConsoleOutputLine *line = get(it);
		ConsoleLineStyle style = console->styles[line->style];

		Rect2 resultRect = uiText(renderBuffer, consoleFont, line->text, textPos, outputLinesAlign, style.textColor, textMaxWidth);
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
	consoleWriteLine(myprintf("> {0}", {commandInput}), CLS_InputEcho);

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
				!it.isDone;
				next(&it))
			{
				Command *command = get(it);

				if (equals(command->name, firstToken))
				{
					foundCommand = true;

					s32 argCount = tokenCount - 1;
					bool tooManyArgs = (argCount > command->maxArgs) && (command->maxArgs != -1);
					if ((argCount < command->minArgs) || tooManyArgs)
					{
						if (command->minArgs == command->maxArgs)
						{
							consoleWriteLine(myprintf("Command '{0}' accepts only {1} argument(s), but {2} given.",
								{firstToken, formatInt(command->minArgs), formatInt(argCount)}), CLS_Error);
						}
						else
						{
							consoleWriteLine(myprintf("Command '{0}' accepts between {1} and {2} arguments, but {3} given.",
								{firstToken, formatInt(command->minArgs), formatInt(command->maxArgs), formatInt(argCount)}), CLS_Error);
						}
					}
					else
					{
						u32 commandStartTime = SDL_GetTicks();
						command->function(console, argCount, arguments);
						u32 commandEndTime = SDL_GetTicks();

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
}

void updateConsole(Console *console, InputState *inputState)
{
	// Keyboard shortcuts for commands
	for (auto it = iterate(&console->commandShortcuts);
		!it.isDone;
		next(&it))
	{
		CommandShortcut *shortcut = get(it);

		if (wasShortcutJustPressed(inputState, shortcut->shortcut))
		{
			consoleHandleCommand(console, shortcut->command);
			console->scrollPos = 0;
		}
	}

	// Show/hide the console
	if (keyJustPressed(inputState, SDLK_TAB))
	{
		if (modifierKeyIsPressed(inputState, KeyMod_Ctrl))
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
		if (updateTextInput(&console->input, inputState))
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
				if (keyJustPressed(inputState, SDLK_UP))
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
				else if (keyJustPressed(inputState, SDLK_DOWN))
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

	while (!isDone(&reader))
	{
		String line = nextLine(&reader);

		String shortcutString, command;

		shortcutString = nextToken(line, &command);

		KeyboardShortcut shortcut = parseKeyboardShortcut(shortcutString);
		if (shortcut.key == SDLK_UNKNOWN)
		{
			error(&reader, "Unrecognised key in keyboard shortcut sequence '{0}'", {shortcutString});
		}
		else
		{
			CommandShortcut *commandShortcut = appendBlank(&console->commandShortcuts);
			commandShortcut->shortcut = shortcut;
			commandShortcut->command = command;
		}
	}
}
