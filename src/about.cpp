#pragma once

void aboutWindowProc(UI::WindowContext *context, void * /*userData*/)
{
	UI::Panel *ui = &context->windowPanel;

	UI::Panel leftColumn = ui->column(64, ALIGN_LEFT, "plain"_s);
	leftColumn.alignWidgets(ALIGN_H_CENTRE);
	leftColumn.addSprite(getSprite("b_hospital"_s));
	leftColumn.end();

	ui->addText(getText("game_title"_s), "title"_s);

	ui->startNewLine(ALIGN_RIGHT);
	ui->addText(getText("game_copyright"_s));

	ui->startNewLine(ALIGN_EXPAND_H);

	if (ui->addTextButton(getText("button_website"_s)))
	{
		openUrlUnsafe("http://samatkins.co.uk");
	}
}

void showAboutWindow()
{
	UI::showWindow(getText("title_about"_s), 300, 200, v2i(0,0), "default"_s, WindowFlags::Unique|WindowFlags::Modal|WindowFlags::AutomaticHeight, aboutWindowProc);
}
