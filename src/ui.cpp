// ui.cpp

void setCursor(UIState *uiState, String cursorName)
{
	DEBUG_FUNCTION();
	
	Asset *newCursorAsset = getAsset(uiState->assets, AssetType_Cursor, cursorName);
	if (newCursorAsset != null)
	{
		uiState->currentCursor = cursorName;
		SDL_SetCursor(newCursorAsset->cursor.sdlCursor);
	}
}

inline void setCursor(UIState *uiState, char *cursorName)
{
	setCursor(uiState, makeString(cursorName));
}

void setCursorVisible(UIState *uiState, bool visible)
{
	uiState->cursorIsVisible = visible;
	SDL_ShowCursor(visible ? 1 : 0);
}

void cacheUIShaders(UIState *uiState, AssetManager *assets)
{
	uiState->textShaderID       = getShader(assets, makeString("textured.glsl"  ))->rendererShaderID;
	uiState->untexturedShaderID = getShader(assets, makeString("untextured.glsl"))->rendererShaderID;
}

void initUiState(UIState *uiState, RenderBuffer *uiBuffer, AssetManager *assets, InputState *input)
{
	*uiState = {};

	uiState->uiBuffer = uiBuffer;
	uiState->assets = assets;
	uiState->input = input;

	uiState->closestDepth = uiBuffer->camera.nearClippingPlane;

	initMemoryArena(&uiState->arena, MB(1));

	uiState->message = {};
	uiState->message.text = pushString(&uiState->arena, 256);
	uiState->message.countdown = -1;

	initialiseArray(&uiState->uiRects, 16);

	initChunkedArray(&uiState->openWindows, &uiState->arena, 16);

	setCursorVisible(uiState, false);
}

Rect2 uiText(UIState *uiState, BitmapFont *font, String text, V2 origin, u32 align, f32 depth, V4 color, f32 maxWidth)
{
	DEBUG_FUNCTION();

	V2 textSize = calculateTextSize(font, text, maxWidth);
	V2 topLeft  = calculateTextPosition(origin, textSize, align);

	Rect2 bounds = rectPosSize(topLeft, textSize);

	drawText(uiState->uiBuffer, font, text, bounds, align, color, uiState->textShaderID);

	return bounds;
}

void basicTooltipWindowProc(WindowContext *context, void * /*userData*/)
{
	window_label(context, context->uiState->tooltipText);
}

void showTooltip(UIState *uiState, WindowProc tooltipProc, void *userData)
{
	static String styleName = makeString("tooltip");
	showWindow(uiState, nullString, 300, 0, styleName, WinFlag_AutomaticHeight | WinFlag_ShrinkWidth | WinFlag_Unique | WinFlag_Tooltip | WinFlag_Headless, tooltipProc, userData);
}

bool uiButton(UIState *uiState, String text, Rect2 bounds, f32 depth, bool active, SDL_Keycode shortcutKey, String tooltip)
{
	DEBUG_FUNCTION();
	
	bool buttonClicked = false;
	V2 mousePos = uiState->uiBuffer->camera.mousePos;
	InputState *input = uiState->input;
	UIButtonStyle *style = findButtonStyle(&uiState->assets->theme, makeString("general"));
	V4 backColor = style->backgroundColor;
	u32 textAlignment = style->textAlignment;

	if (!uiState->mouseInputHandled && contains(bounds, mousePos))
	{
		uiState->mouseInputHandled = true;

		// Mouse pressed: must have started and currently be inside the bounds to show anything
		// Mouse unpressed: show hover if in bounds
		if (mouseButtonPressed(input, MouseButton_Left))
		{
			if (contains(bounds, getClickStartPos(input, MouseButton_Left, &uiState->uiBuffer->camera)))
			{
				backColor = style->pressedColor;
			}
		}
		else
		{
			if (mouseButtonJustReleased(input, MouseButton_Left)
			 && contains(bounds, getClickStartPos(input, MouseButton_Left, &uiState->uiBuffer->camera)))
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

	drawRect(uiState->uiBuffer, bounds, depth, uiState->untexturedShaderID, backColor);
	V2 textOrigin = alignWithinRectangle(bounds, textAlignment, style->padding);
	uiText(uiState, getFont(uiState->assets, style->fontName), text, textOrigin, textAlignment, depth + 1,
			style->textColor);

	// Keyboard shortcut!
	if ((shortcutKey != SDLK_UNKNOWN)
	&& keyJustPressed(input, shortcutKey))
	{
		buttonClicked = true;
	}

	return buttonClicked;
}

bool uiMenuButton(UIState *uiState, String text, Rect2 bounds, f32 depth, s32 menuID, SDL_Keycode shortcutKey, String tooltip)
{
	DEBUG_FUNCTION();
	
	bool currentlyOpen = uiState->openMenu == menuID;
	if (uiButton(uiState, text, bounds, depth, currentlyOpen, shortcutKey, tooltip))
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
			UIMessageStyle *style = findMessageStyle(&uiState->assets->theme, makeString("general"));

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

			f32 depth = uiState->closestDepth - 100.0f;

			V2 origin = v2(uiState->uiBuffer->camera.size.x * 0.5f, uiState->uiBuffer->camera.size.y - 8.0f);

			RenderItem_DrawThing *backgroundRI = appendRenderItem(uiState->uiBuffer);

			Rect2 labelRect = uiText(uiState, getFont(uiState->assets, style->fontName), uiState->message.text, origin,
										 ALIGN_H_CENTRE | ALIGN_BOTTOM, depth + 1.0f, textColor);

			labelRect = expand(labelRect, style->padding);

			drawRect(backgroundRI, labelRect, depth, uiState->untexturedShaderID, backgroundColor);
		}
	}
}

void drawScrollBar(RenderBuffer *uiBuffer, V2 topLeft, f32 height, f32 scrollPercent, V2 knobSize, f32 depth, V4 knobColor, s32 shaderID)
{
	knobSize.y = min(knobSize.y, height); // force knob to fit
	f32 knobTravelableH = height - knobSize.y;
	f32 scrollY = scrollPercent * knobTravelableH;
	Rect2 knobRect = rectXYWH(topLeft.x, topLeft.y + scrollY, knobSize.x, knobSize.y);
	drawRect(uiBuffer, knobRect, depth, shaderID, knobColor);
}

inline void uiCloseMenus(UIState *uiState)
{
	uiState->openMenu = 0;
}
