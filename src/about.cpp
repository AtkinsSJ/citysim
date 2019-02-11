#pragma once

void aboutWindowProc(WindowContext *context, void *userData)
{
	// shut up the warning
	userData = userData;

	window_label(context, LocalString("Some kind of city simulator game"), "title");
	window_label(context, LocalString("© Copyright Samuel Atkins 20XX"));
	if (window_button(context, LocalString("Website")))
	{
		openUrlUnsafe("http://samatkins.co.uk");
	}
}

void showAboutWindow(UIState *uiState)
{
	showWindow(uiState, LocalString("About"), 300, 200, stringFromChars("general"), WinFlag_Unique|WinFlag_Modal|WinFlag_AutomaticHeight, aboutWindowProc, null);
}