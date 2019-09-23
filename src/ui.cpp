// ui.cpp

void initUIState(UIState *uiState, MemoryArena *arena)
{
	*uiState = {};

	uiState->message = {};
	uiState->message.text = pushString(arena, 256);
	uiState->message.countdown = -1;

	initChunkedArray(&uiState->uiRects, arena, 64);

	initChunkedArray(&uiState->openWindows, arena, 64);
}

Rect2I uiText(RenderBuffer *renderBuffer, BitmapFont *font, String text, V2I origin, u32 align, V4 color, s32 maxWidth)
{
	DEBUG_FUNCTION();

	V2I textSize = calculateTextSize(font, text, maxWidth);
	V2I topLeft  = calculateTextPosition(origin, textSize, align);

	Rect2I bounds = irectPosSize(topLeft, textSize);

	drawText(renderBuffer, font, text, bounds, align, color, renderer->shaderIds.text);

	return bounds;
}

void basicTooltipWindowProc(WindowContext *context, void * /*userData*/)
{
	window_label(context, context->uiState->tooltipText);
}

void showTooltip(UIState *uiState, WindowProc tooltipProc, void *userData)
{
	static String styleName = "tooltip"s;
	showWindow(uiState, nullString, 300, 0, v2i(0,0), styleName, WinFlag_AutomaticHeight | WinFlag_ShrinkWidth | WinFlag_Unique | WinFlag_Tooltip | WinFlag_Headless, tooltipProc, userData);
}

V2I calculateButtonSize(String text, UIButtonStyle *buttonStyle, s32 maxWidth)
{
	s32 doublePadding = (buttonStyle->padding * 2);
	s32 textMaxWidth = (maxWidth == 0) ? 0 : (maxWidth - doublePadding);

	BitmapFont *font = getFont(buttonStyle->fontName);
	V2I textSize = calculateTextSize(font, text, textMaxWidth);

	V2I result = v2i(textSize.x + doublePadding, textSize.y + doublePadding);

	return result;
}

bool uiButton(UIState *uiState, String text, Rect2I bounds, UIButtonStyle *style, bool active, SDL_Keycode shortcutKey, String tooltip)
{
	DEBUG_FUNCTION();
	
	if (style == null)
	{
		style = findButtonStyle(&assets->theme, "default"s);
	}
	
	bool buttonClicked = false;
	V2I mousePos = v2i(renderer->uiCamera.mousePos);

	V4 backColor = style->backgroundColor;
	u32 textAlignment = style->textAlignment;

	if (!uiState->mouseInputHandled && contains(bounds, mousePos))
	{
		uiState->mouseInputHandled = true;

		// Mouse pressed: must have started and currently be inside the bounds to show anything
		// Mouse unpressed: show hover if in bounds
		if (mouseButtonPressed(MouseButton_Left))
		{
			if (contains(bounds, getClickStartPos(MouseButton_Left, &renderer->uiCamera)))
			{
				backColor = style->pressedColor;
			}
		}
		else
		{
			if (mouseButtonJustReleased(MouseButton_Left)
			 && contains(bounds, getClickStartPos(MouseButton_Left, &renderer->uiCamera)))
			{
				buttonClicked = true;
			}
			backColor = style->hoverColor;
		}

		if (tooltip.length)
		{
			uiState->tooltipText = tooltip;
			showTooltip(uiState, basicTooltipWindowProc, null);
		}
	}
	else if (active)
	{
		backColor = style->hoverColor;
	}

	drawSingleRect(&renderer->uiBuffer, bounds, renderer->shaderIds.untextured, backColor);
	V2I textOrigin = alignWithinRectangle(bounds, textAlignment, style->padding);
	uiText(&renderer->uiBuffer, getFont(style->fontName), text, textOrigin, textAlignment, style->textColor);

	// Keyboard shortcut!
	if ((shortcutKey != SDLK_UNKNOWN)
	&& keyJustPressed(shortcutKey))
	{
		buttonClicked = true;
	}

	return buttonClicked;
}

bool uiMenuButton(UIState *uiState, String text, Rect2I bounds, s32 menuID, UIButtonStyle *style, SDL_Keycode shortcutKey, String tooltip)
{
	DEBUG_FUNCTION();
	
	bool currentlyOpen = uiState->openMenu == menuID;
	if (uiButton(uiState, text, bounds, style, currentlyOpen, shortcutKey, tooltip))
	{
		if (currentlyOpen)
		{
			uiState->openMenu = 0;
			currentlyOpen = false;
		}
		else
		{
			uiState->openMenu = menuID;
			currentlyOpen = true;

			// NB: Do all menu-state-initialisation here!
		}
	}

	return currentlyOpen;
}

void pushUiMessage(UIState *uiState, String message)
{
	copyString(message, &uiState->message.text);
	uiState->message.countdown = uiMessageDisplayTime;
}

void drawUiMessage(UIState *uiState)
{
	DEBUG_FUNCTION();
	
	if (uiState->message.countdown > 0)
	{
		uiState->message.countdown -= SECONDS_PER_FRAME;

		if (uiState->message.countdown > 0)
		{
			UIMessageStyle *style = findMessageStyle(&assets->theme, "default"s);

			f32 t = (f32)uiState->message.countdown / uiMessageDisplayTime;

			V4 backgroundColor = style->backgroundColor;
			V4 textColor = style->textColor;

			if (t < 0.2f)
			{
				// Fade out
				f32 tt = t / 0.2f;
				backgroundColor *= lerp<f32>(0, backgroundColor.a, tt);
				textColor *= lerp<f32>(0, textColor.a, tt);
			}
			else if (t > 0.8f)
			{
				// Fade in
				f32 tt = (t - 0.8f) / 0.2f;
				backgroundColor *= lerp<f32>(backgroundColor.a, 0, tt);
				textColor *= lerp<f32>(textColor.a, 0, tt);
			}

			V2I origin = v2i(floor_s32(renderer->uiCamera.size.x / 2), floor_s32(renderer->uiCamera.size.y - 8));

			RenderItem_DrawSingleRect *backgroundRI = appendDrawRectPlaceholder(&renderer->uiBuffer, renderer->shaderIds.untextured);

			Rect2I labelRect = uiText(&renderer->uiBuffer, getFont(style->fontName), uiState->message.text, origin,
										 ALIGN_H_CENTRE | ALIGN_BOTTOM, textColor);

			labelRect = expand(labelRect, style->padding);

			fillDrawRectPlaceholder(backgroundRI, labelRect, backgroundColor);
		}
	}
}

void drawScrollBar(RenderBuffer *uiBuffer, V2I topLeft, s32 height, f32 scrollPercent, V2I knobSize, V4 knobColor, s8 shaderID)
{
	knobSize.y = min(knobSize.y, height); // force knob to fit
	s32 scrollY = round_s32(scrollPercent * (height - knobSize.y));
	Rect2 knobRect = rectXYWHi(topLeft.x, topLeft.y + scrollY, knobSize.x, knobSize.y);
	drawSingleRect(uiBuffer, knobRect, shaderID, knobColor);
}

inline void uiCloseMenus(UIState *uiState)
{
	uiState->openMenu = 0;
}

PopupMenu beginPopupMenu(s32 x, s32 y, s32 width, V4 backgroundColor)
{
	PopupMenu result = {};

	result.origin = v2i(x, y);
	result.width = width;

	result.padding = 4; // TODO: Define this in styles!
	result.backgroundColor = backgroundColor;
	result.backgroundRect = appendDrawRectPlaceholder(&renderer->uiBuffer, renderer->shaderIds.untextured);
	result.currentYOffset = result.padding;

	return result;
}

bool popupMenuButton(UIState *uiState, PopupMenu *menu, String text, UIButtonStyle *style, bool isActive)
{
	V2I buttonSize = calculateButtonSize(text, style, menu->width - (menu->padding * 2));

	Rect2I buttonRect = irectXYWH(menu->origin.x + menu->padding,
								menu->origin.y + menu->currentYOffset,
								buttonSize.x, buttonSize.y);

	bool result = uiButton(uiState, text, buttonRect, style, isActive);

	menu->currentYOffset += buttonRect.h + menu->padding;

	return result;
}

void endPopupMenu(UIState *uiState, PopupMenu *menu)
{
	Rect2I menuRect = irectXYWH(menu->origin.x, menu->origin.y, menu->width, menu->currentYOffset);
	append(&uiState->uiRects, menuRect);
	fillDrawRectPlaceholder(menu->backgroundRect, menuRect, menu->backgroundColor);
}
