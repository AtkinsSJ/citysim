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
//    UI::Panel a = UI::Panel(...);
//    UI::Panel b = UI::Panel(...);
//    a.addLabel("First"_s);
//    b.addLabel("Second"_s);
//    a.addLabel("Third"_s);
//
// will not work, as "Third" will be clipped to the boundaries of b, not a! But as long as
// you avoid code like that, it's fine - adding all the 'a' widgets, then all of 'b', or
// vice-versa.
//
// - Sam, 04/02/2021
//

namespace UI {
namespace PanelFlags {

enum {
    LayoutBottomToTop = 1 << 0,
    AllowClickThrough = 1 << 1,
    HideWidgets = 1 << 2, // Widgets are not updated or rendered, just laid out
};

}

struct Panel {
    Panel(Rect2I bounds, PanelStyle* style = null, u32 flags = 0, RenderBuffer* renderBuffer = &renderer->uiBuffer);
    Panel(Rect2I bounds, String styleName, u32 flags = 0, RenderBuffer* renderBuffer = &renderer->uiBuffer)
        : Panel(bounds, getStyle<PanelStyle>(styleName), flags, renderBuffer)
    {
    }

    // Configuration functions, which should be called before adding any widgets
    void enableHorizontalScrolling(ScrollbarState* hScrollbar);
    void enableVerticalScrolling(ScrollbarState* vScrollbar, bool expandWidth = false);

    // Add stuff to the panel
    bool addTextButton(String text, ButtonState state = Button_Normal, String styleName = nullString);
    bool addImageButton(Sprite* sprite, ButtonState state = Button_Normal, String styleName = nullString);

    void addCheckbox(bool* checked, String styleName = nullString);

    template<typename T>
    void addDropDownList(Array<T>* listOptions, s32* currentSelection, String (*getDisplayName)(T* data), String styleName = nullString);

    void addLabel(String text, String styleName = nullString);

    void addRadioButton(s32* currentValue, s32 myValue, String styleName = nullString);
    template<typename T>
    void addRadioButtonGroup(Array<T>* listOptions, s32* currentSelection, String (*getDisplayName)(T* data), String styleName = nullString, String labelStyleName = nullString);

    template<typename T>
    void addSlider(T* currentValue, T minValue, T maxValue, String styleName = nullString);

    void addSprite(Sprite* sprite, s32 width = -1, s32 height = -1);

    bool addTextInput(TextInput* textInput, String styleName = nullString);

    // Add a blank rectangle as if it were a widget. (So, leaving padding between
    // it and other widgets.) The bounds are returned so you can draw your own
    // contents.
    Rect2I addBlank(s32 width, s32 height = 0);

    // Layout options
    void alignWidgets(u32 alignment);
    void startNewLine(u32 hAlignment = 0);

    // Slice the remaining content area in two, with one part being the new Panel,
    // and the rest becoming the existing panel's content area.
    Panel row(s32 height, Alignment vAlignment, String styleName = nullString);
    Panel column(s32 width, Alignment hAlignment, String styleName = nullString);

    void end(bool shinkToContentHeight = false, bool shrinkToContentWidth = false);

    // "Private"
    void prepareForWidgets();
    template<typename Func>
    Rect2I calculateWidgetBounds(Func calculateSize);
    Rect2I calculateWidgetBounds(V2I size);
    void completeWidget(V2I widgetSize);
    PanelStyle* getPanelStyle(String styleName);

    // Call after modifying the contentArea. Updates the positions fields to match.
    void updateLayoutPosition();

    // Flags, and bool versions for easier access
    u32 flags;
    bool allowClickThrough;
    bool hideWidgets; // Widgets are not updated or rendered, just laid out
    bool layoutBottomToTop;

    bool hasAddedWidgets;

    RenderBuffer* renderBuffer;

    Rect2I bounds;
    Rect2I contentArea;
    u32 widgetAlignment;

    ScrollbarState* hScrollbar;
    Rect2I hScrollbarBounds;
    ScrollbarState* vScrollbar;
    Rect2I vScrollbarBounds;

    // Relative to contentArea
    s32 currentLeft;
    s32 currentRight;
    s32 currentTop;
    s32 currentBottom;

    s32 largestItemWidth;
    s32 largestItemHeightOnLine;
    s32 largestLineWidth;

    PanelStyle* style;
    Drawable background;
};

}
