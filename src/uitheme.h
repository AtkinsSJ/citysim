#pragma once

struct UIButtonStyle
{
	FontAssetType font;
	V4 textColor;

	V4 backgroundColor;
	V4 hoverColor;
	V4 pressedColor;
};

struct UILabelStyle
{
	FontAssetType font;
	V4 textColor;
};

struct UITextBoxStyle
{
	FontAssetType font;
	V4 textColor;
	V4 backgroundColor;
};

struct UITooltipStyle
{
	FontAssetType font;
	V4 textColorNormal;
	V4 textColorBad;

	V4 backgroundColor;
	f32 borderPadding;
	f32 depth;
};

struct UIMessageStyle
{
	FontAssetType font;
	V4 textColor;

	V4 backgroundColor;
	f32 borderPadding;
	f32 depth;
};

struct UIWindowStyle
{
	FontAssetType titleFont;
	f32 titleBarHeight;
	V4 titleColor;
	V4 titleBarColorActive;
	V4 titleBarColorInactive;
	V4 backgroundColorActive;
	V4 backgroundColorInactive;
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
