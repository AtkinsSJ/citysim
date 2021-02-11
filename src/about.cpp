#pragma once

void aboutWindowProc(WindowContext *context, void * /*userData*/)
{
	UIPanel *ui = &context->windowPanel;
	ui->addText(getText("game_title"_s), "title"_s);

	ui->startNewLine(ALIGN_RIGHT);
	ui->addText(getText("game_copyright"_s));

	ui->startNewLine(ALIGN_EXPAND_H);

	if (ui->addButton(getText("button_website"_s)))
	{
		openUrlUnsafe("http://samatkins.co.uk");
	}
}

void showAboutWindow(UIState *uiState)
{
	showWindow(uiState, getText("title_about"_s), 300, 200, v2i(0,0), "default"_s, WinFlag_Unique|WinFlag_Modal|WinFlag_AutomaticHeight, aboutWindowProc, null);
}
