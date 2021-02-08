#pragma once

// Going to try out using a more OOP style.
// I didn't like putting "window_" at the start of all the Window-UI functions, or
// having to pass it as the parameter all the time.
// Maybe this will feel awkward too, who knows.

//
// NB: Panels push a BeginScissor when their first widget is added.
// This means that you can't alternate between adding widgets to two different ones, as 
// they'll both use the scissor of whichever Panel had its first child added last. eg this:
//
//    UIPanel a = UIPanel(...);
//    UIPanel b = UIPanel(...);
//    a.addText("First"_s);
//    b.addText("Second"_s);
//    a.addText("Third"_s);
//
// will not work, as "Third" will be clipped to the boundaries of b, not a! But as long as
// you avoid code like that, it's fine - adding all the 'a' widgets, then all of 'b', or
// vice-versa.
//
// - Sam, 04/02/2021
//
struct UIPanel
{
	UIPanel(Rect2I bounds, UIPanelStyle *style = null, bool topToBottom = true, bool isTopLevel = true);
	UIPanel(Rect2I bounds, String styleName, bool topToBottom = true)
		: UIPanel(bounds, findPanelStyle(&assets->theme, styleName), topToBottom) {}

	// Configuration functions, which should be called before adding any widgets
	void enableHorizontalScrolling(ScrollbarState *hScrollbar);
	void enableVerticalScrolling(ScrollbarState *vScrollbar, bool expandWidth=false);

	// Add stuff to the panel
	void addText(String text, String styleName = nullString);
	bool addButton(String text, ButtonState state = Button_Normal, String styleName = nullString);
	bool addTextInput(TextInput *textInput, String styleName = nullString);

	// Layout options
	void alignWidgets(u32 alignment);
	void startNewLine(u32 hAlignment = 0);

	// Slice the remaining content area in two, with one part being the new UIPanel,
	// and the rest becoming the existing panel's content area.
	UIPanel row(s32 height, Alignment vAlignment, String styleName = nullString);
	UIPanel column(s32 width, Alignment hAlignment, String styleName = nullString);

	void end(bool shinkToContentHeight = false);

	// "Private"
	void prepareForWidgets();
	Rect2I getCurrentLayoutPosition();
	void completeWidget(V2I widgetSize);

	// Call after modifying the contentArea. Updates the positions fields to match.
	void updateLayoutPosition();

	bool isTopLevel;
	bool hasAddedWidgets;

	Rect2I bounds;
	Rect2I contentArea;
	u32 widgetAlignment;

	ScrollbarState *hScrollbar;
	Rect2I hScrollbarBounds;
	ScrollbarState *vScrollbar;
	Rect2I vScrollbarBounds;

	// Relative to contentArea
	bool topToBottom;
	s32 currentLeft;
	s32 currentRight;
	s32 currentTop;
	s32 currentBottom;
	
	s32 largestItemWidth;
	s32 largestItemHeightOnLine;

	UIPanelStyle *style;
	RenderItem_DrawSingleRect *backgroundPlaceholder;
};
