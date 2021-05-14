#pragma once

void aboutWindowProc(WindowContext *context, void * /*userData*/)
{
	UIPanel *ui = &context->windowPanel;

	UIPanel leftColumn = ui->column(64, ALIGN_LEFT, "plain"_s);
	leftColumn.alignWidgets(ALIGN_H_CENTRE);
	leftColumn.addSprite(getSprite("b_hospital"_s));
	leftColumn.end();

	ui->addText(getText("game_title"_s), "title"_s);

	ui->startNewLine(ALIGN_RIGHT);
	ui->addText(getText("game_copyright"_s));

	ui->startNewLine(ALIGN_EXPAND_H);

	if (ui->addButton(getText("button_website"_s)))
	{
		openUrlUnsafe("http://samatkins.co.uk");
	}
}

void showAboutWindow()
{
	UI::showWindow(getText("title_about"_s), 300, 200, v2i(0,0), "default"_s, WinFlag_Unique|WinFlag_Modal|WinFlag_AutomaticHeight, aboutWindowProc);
}
