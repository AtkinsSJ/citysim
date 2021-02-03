#pragma once

UIPanel::UIPanel(Rect2I bounds, UIPanelStyle *style)
{
	this->bounds = bounds;
	this->contentArea = shrink(bounds, style->margin);

	// TODO: make this a parameter to pass in?
	// TODO: maybe make it a style property too?
	this->widgetAlignment = ALIGN_TOP | ALIGN_EXPAND_H;

	this->currentLeft= 0;
	this->currentRight = this->contentArea.w;
	this->currentY = 0;

	this->largestItemWidth = 0;
	this->largestItemHeightOnLine = 0;
}


void UIPanel::endPanel()
{
	// Fill in the background


	// Draw the scrollbar if we have one


	// Clear any scissor stuff
}