#pragma once

static Console theConsole;

void initConsole(MemoryArena *debugArena, f32 openHeight, f32 maximisedHeight, f32 openSpeed)
{
	Console *console = &theConsole;

	// NB: Console->style is initialised later, in updateConsole(), because initConsole() happens
	// before the asset system is initialised!

	console->currentHeight = 0;

	console->openHeight = openHeight;
	console->maximisedHeight = maximisedHeight;
	console->openSpeed = openSpeed;

	console->input = UI::newTextInput(debugArena, consoleLineLength);
	initChunkedArray(&console->inputHistory, debugArena, 256);
	console->inputHistoryCursor = -1;

	initChunkedArray(&console->outputLines, debugArena, 1024);
	initScrollbar(&console->scrollbar, false);

	initChunkedArray(&console->commandShortcuts, &globalAppState.systemArena, 64);

	initCommands(console);
	consoleWriteLine(myprintf("Loaded {0} commands. Type 'help' to list them."_s, {formatInt(console->commands.count)}), CLS_Default);

	globalConsole = console;

	consoleWriteLine("GREETINGS PROFESSOR FALKEN.\nWOULD YOU LIKE TO PLAY A GAME?"_s);
}

void updateConsole(Console *console)
{
	bool scrollToBottom = false;

	// Late-init the console style
	if (console->style.type != AssetType_ConsoleStyle)
	{
		console->style = getAssetRef(AssetType_ConsoleStyle, "default"_s);
	}

	// Keyboard shortcuts for commands
	for (auto it = console->commandShortcuts.iterate();
		it.hasNext();
		it.next())
	{
		CommandShortcut *shortcut = it.get();

		if (wasShortcutJustPressed(shortcut->shortcut))
		{
			consoleHandleCommand(console, shortcut->command);
			scrollToBottom = true;
		}
	}

	// Show/hide the console
	if (keyJustPressed(SDLK_BACKQUOTE))
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
			scrollToBottom = true;
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
				scrollToBottom = true;
			}
		}
	}

	if (console->currentHeight != console->targetHeight)
	{
		console->currentHeight = approach(console->currentHeight, console->targetHeight, console->openSpeed * globalAppState.deltaTime);
	}

	// This is a little hacky... I think we want the console to ALWAYS consume input if it is open.
	// Other things can't take control from it.
	// Maybe this is a terrible idea? Not sure, but we'll see.
	// - Sam, 16/01/2020
	if (console->currentHeight > 0)
	{
		captureInput(&console->input);
	}
	else
	{
		releaseInput(&console->input);
	}

	if (hasCapturedInput(&console->input))
	{
		if (UI::updateTextInput(&console->input))
		{
			consoleHandleCommand(console, console->input.toString());
			scrollToBottom = true;
			console->input.clear();
		}
		else
		{
			// Tab completion
			if (keyJustPressed(SDLK_TAB))
			{
				String wordToComplete = console->input.getLastWord();
				if (!wordToComplete.isEmpty())
				{
					// Search through our commands to find one that matches
					// Eventually, we might want to cache an array of commands, but that's a low priority for now
					String completeCommand = nullString;

					for (auto it = console->commands.iterate();
						it.hasNext();
						it.next())
					{
						Command *c = it.get();
						if (c->name.startsWith(wordToComplete))
						{
							completeCommand = c->name;
							break;
						}
					}

					if (!completeCommand.isEmpty())
					{
						String completion = makeString(completeCommand.chars + wordToComplete.length, completeCommand.length - wordToComplete.length);
						console->input.insert(completion);
					}

					if (completeCommand.isEmpty())
					{
						logDebug("Word to complete: `{0}`, but no matches found"_s, {wordToComplete});
					}
					else
					{
						logDebug("Word to complete: `{0}`, completed to: `{1}`"_s, {wordToComplete, completeCommand});
					}
				}
			}
			// Command history
			else if (keyJustPressed(SDLK_UP))
			{
				if (console->inputHistory.count > 0)
				{
					if (console->inputHistoryCursor == -1)
					{
						console->inputHistoryCursor = truncate32(console->inputHistory.count - 1);
					}
					else
					{
						console->inputHistoryCursor = max(console->inputHistoryCursor - 1, 0);
					}

					console->input.clear();
					String oldInput = *console->inputHistory.get(console->inputHistoryCursor);
					console->input.append(oldInput);
				}
			}
			// Command history
			else if (keyJustPressed(SDLK_DOWN))
			{
				if (console->inputHistory.count > 0)
				{
					if (console->inputHistoryCursor == -1)
					{
						console->inputHistoryCursor = truncate32(console->inputHistory.count - 1);
					}
					else
					{
						console->inputHistoryCursor = truncate32(min(console->inputHistoryCursor + 1, (s32)console->inputHistory.count - 1));
					}

					console->input.clear();
					String oldInput = *console->inputHistory.get(console->inputHistoryCursor);
					console->input.append(oldInput);
				}
			}
		}
	}

	if (console->currentHeight > 0)
	{
		UIConsoleStyle *consoleStyle = findStyle<UIConsoleStyle>(&console->style);
		UIScrollbarStyle *scrollbarStyle = findStyle<UIScrollbarStyle>(&consoleStyle->scrollbarStyle);
		Rect2I scrollbarBounds = getConsoleScrollbarBounds(console);
		if (hasPositiveArea(scrollbarBounds))
		{
			s32 fontLineHeight = getFont(&consoleStyle->font)->lineHeight;
			s32 contentHeight = ((console->outputLines.count-1) * fontLineHeight) + scrollbarBounds.h;
			if (scrollToBottom)
			{
				console->scrollbar.scrollPercent = 1.0f;
			}

			console->scrollbar.mouseWheelStepSize = 3 * fontLineHeight;
			updateScrollbar(&console->scrollbar, contentHeight, scrollbarBounds, scrollbarStyle);
		}
	}
}

void renderConsole(Console *console)
{
	RenderBuffer *renderBuffer = &renderer->debugBuffer;

	s32 actualConsoleHeight = floor_s32(console->currentHeight * renderer->uiCamera.size.y);

	s32 screenWidth = inputState->windowWidth;

	UIConsoleStyle   *consoleStyle   = findStyle<UIConsoleStyle>(&console->style);
	UIScrollbarStyle *scrollbarStyle = findStyle<UIScrollbarStyle>(&consoleStyle->scrollbarStyle);
	UITextInputStyle *textInputStyle = findStyle<UITextInputStyle>(&consoleStyle->textInputStyle);

	V2I textInputSize = UI::calculateTextInputSize(&console->input, textInputStyle, screenWidth);
	V2I textInputPos  = v2i(0, actualConsoleHeight - textInputSize.y);
	Rect2I textInputBounds = irectPosSize(textInputPos, textInputSize);
	UI::drawTextInput(renderBuffer, &console->input, textInputStyle, textInputBounds);

	V2I textPos = v2i(consoleStyle->padding, textInputBounds.y);
	s32 textMaxWidth = screenWidth - (2*consoleStyle->padding);

	s32 heightOfOutputArea = textPos.y;

	Rect2I consoleBackRect = irectXYWH(0,0,screenWidth, heightOfOutputArea);
	UIDrawable(&consoleStyle->background).draw(renderBuffer, consoleBackRect);

	Rect2I scrollbarBounds = getConsoleScrollbarBounds(console);
	if (hasPositiveArea(scrollbarBounds))
	{
		drawScrollbar(renderBuffer, &console->scrollbar, scrollbarBounds, scrollbarStyle);
	}

	textPos.y -= consoleStyle->padding;

	// print output lines
	BitmapFont *consoleFont = getFont(&consoleStyle->font);
	s32 scrollLinePos = clamp(floor_s32(console->scrollbar.scrollPercent * console->outputLines.count), 0, console->outputLines.count - 1);
	s32 outputLinesAlign = ALIGN_LEFT | ALIGN_BOTTOM;
	for (auto it = console->outputLines.iterate(scrollLinePos, false, true);
		it.hasNext();
		it.next())
	{
		ConsoleOutputLine *line = it.get();
		V4 outputTextColor = consoleStyle->outputTextColor[line->style];

		Rect2I resultRect = UI::drawText(renderBuffer, consoleFont, line->text, textPos, outputLinesAlign, outputTextColor, textMaxWidth);
		textPos.y -= resultRect.h;

		// If we've gone off the screen, stop!
		if ((textPos.y < 0) || (textPos.y > renderer->uiCamera.size.y))
		{
			break;
		}
	}
}

void loadConsoleKeyboardShortcuts(Console *console, Blob data, String filename)
{
	LineReader reader = readLines(filename, data);

	console->commandShortcuts.clear();

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
			CommandShortcut *commandShortcut = console->commandShortcuts.appendBlank();
			commandShortcut->shortcut = shortcut;
			commandShortcut->command = command;
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
		console->inputHistory.append(pushString(console->inputHistory.memoryArena, commandInput));
		console->inputHistoryCursor = -1;

		s32 tokenCount = countTokens(commandInput);
		if (tokenCount > 0)
		{
			String arguments;
			String firstToken = nextToken(commandInput, &arguments);

			// Find the command
			Command *command = null;
			for (auto it = console->commands.iterate();
				it.hasNext();
				it.next())
			{
				Command *c = it.get();
				if (equals(c->name, firstToken))
				{
					command = c;
					break;
				}
			}

			// Run the command
			if (command != null)
			{
				s32 argCount = tokenCount - 1;
				bool tooManyArgs = (argCount > command->maxArgs) && (command->maxArgs != -1);
				if ((argCount < command->minArgs) || tooManyArgs)
				{
					if (command->minArgs == command->maxArgs)
					{
						consoleWriteLine(myprintf("Command '{0}' requires exactly {1} argument(s), but {2} given."_s,
							{firstToken, formatInt(command->minArgs), formatInt(argCount)}), CLS_Error);
					}
					else if (command->maxArgs == -1)
					{
						consoleWriteLine(myprintf("Command '{0}' requires at least {1} argument(s), but {2} given."_s,
							{firstToken, formatInt(command->minArgs), formatInt(argCount)}), CLS_Error);
					}
					else
					{
						consoleWriteLine(myprintf("Command '{0}' requires between {1} and {2} arguments, but {3} given."_s,
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
			}
			else
			{
				consoleWriteLine(myprintf("I don't understand '{0}'. Try 'help' for a list of commands."_s, {firstToken}), CLS_Error);
			}
		}
	}
}

void consoleWriteLine(String text, ConsoleLineStyleID style)
{
	if (globalConsole)
	{
		ConsoleOutputLine *line = globalConsole->outputLines.appendBlank();
		line->style = style;
		line->text = pushString(globalConsole->outputLines.memoryArena, text);
	}
}

Rect2I getConsoleScrollbarBounds(Console *console)
{
	UIConsoleStyle   *consoleStyle   = findStyle<UIConsoleStyle>  (&console->style);
	UIScrollbarStyle *scrollbarStyle = findStyle<UIScrollbarStyle>(&consoleStyle->scrollbarStyle);
	UITextInputStyle *textInputStyle = findStyle<UITextInputStyle>(&consoleStyle->textInputStyle);

	V2I textInputSize = UI::calculateTextInputSize(&console->input, textInputStyle, inputState->windowWidth);

	Rect2I scrollbarBounds = irectXYWH(inputState->windowWidth - scrollbarStyle->width, 0, scrollbarStyle->width, floor_s32(console->currentHeight * inputState->windowHeight) - textInputSize.y);

	return scrollbarBounds;
}
