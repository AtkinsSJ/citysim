#pragma once

void aboutWindowProc(WindowContext *context, void *userData)
{
	// shut up the warning
	userData = userData;

	window_label(context, LOCAL("game_title"), "title");
	window_label(context, LOCAL("game_copyright"));

	context->alignment = ALIGN_RIGHT;
	if (window_button(context, LOCAL("button_website")))
	{
		openUrlUnsafe("http://samatkins.co.uk");
	}
}

void showAboutWindow(UIState *uiState)
{
	showWindow(uiState, LOCAL("title_about"), 300, 200, v2i(0,0), "default"s, WinFlag_Unique|WinFlag_Modal|WinFlag_AutomaticHeight, aboutWindowProc, null);
}