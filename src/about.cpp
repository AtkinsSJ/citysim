#pragma once

void aboutWindowProc(WindowContext *context, void * /*userData*/)
{
	window_label(context, getText("game_title"_s), "title"_s);

	window_startNewLine(context, ALIGN_RIGHT);
	window_label(context, getText("game_copyright"_s));

	window_startNewLine(context, ALIGN_EXPAND_H);
	if (window_button(context, getText("button_website"_s)))
	{
		openUrlUnsafe("http://samatkins.co.uk");
	}
}

void showAboutWindow(UIState *uiState)
{
	showWindow(uiState, getText("title_about"_s), 300, 200, v2i(0,0), "default"_s, WinFlag_Unique|WinFlag_Modal|WinFlag_AutomaticHeight, aboutWindowProc, null);
}
