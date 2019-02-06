#pragma once

struct UIButtonStyle
{
	s32 fontID;
	V4 textColor;

	V4 backgroundColor;
	V4 hoverColor;
	V4 pressedColor;

	f32 padding;
};

struct UILabelStyle
{
	s32 fontID;
	V4 textColor;
};

struct UITextBoxStyle
{
	s32 fontID;
	V4 textColor;
	V4 backgroundColor;
};

struct UITooltipStyle
{
	s32 fontID;
	V4 textColorNormal;
	V4 textColorBad;

	V4 backgroundColor;
	f32 borderPadding;
	f32 depth;
};

struct UIMessageStyle
{
	s32 fontID;
	V4 textColor;

	V4 backgroundColor;
	f32 borderPadding;
	f32 depth;
};

struct UIWindowStyle
{
	f32 titleBarHeight;
	V4 titleBarColor;
	V4 titleBarColorInactive;
	s32 titleFontID;
	V4 titleColor;
	V4 titleBarButtonHoverColor;

	V4 backgroundColor;
	V4 backgroundColorInactive;

	f32 contentPadding;

	UIButtonStyle *buttonStyle;
	UILabelStyle *labelStyle;
};

struct UITheme
{
	V4 overlayColor;

	UIButtonStyle buttonStyle;
	UILabelStyle labelStyle;
	UITooltipStyle tooltipStyle;
	UIMessageStyle uiMessageStyle;
	UITextBoxStyle textBoxStyle;
	UIWindowStyle windowStyle;
};
