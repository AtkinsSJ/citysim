#pragma once

struct UIButtonStyle
{
	String name;

	s32 fontID;
	V4 textColor;
	u32 textAlignment;

	V4 backgroundColor;
	V4 hoverColor;
	V4 pressedColor;

	f32 padding;
};

struct UILabelStyle
{
	String name;
	
	s32 fontID;
	V4 textColor;
};

struct UITextBoxStyle
{
	String name;
	
	s32 fontID;
	V4 textColor;
	V4 backgroundColor;
};

struct UITooltipStyle
{
	String name;
	
	s32 fontID;
	V4 textColor;

	V4 backgroundColor;
	f32 padding;

	V2 offsetFromCursor;
};

struct UIMessageStyle
{
	String name;
	
	s32 fontID;
	V4 textColor;

	V4 backgroundColor;
	f32 padding;
};

struct UIWindowStyle
{
	String name;
	
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
};
