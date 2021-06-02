#pragma once

namespace WindowFlags
{
	enum
	{
		AutomaticHeight = 1 << 0, // Height determined by content
		Unique          = 1 << 1, // Only one window with the same WindowProc will be allowed. A new one will replace the old.
		Modal           = 1 << 2,
		Tooltip         = 1 << 3, // Follows the mouse cursor
		Headless        = 1 << 4, // No title bar
		ShrinkWidth     = 1 << 5, // Reduces the width to match the content of the window
		Pause           = 1 << 6, // Pauses the game while open
	};
};

namespace UI
{
	// NB: A WindowContext only exists for a single call of a WindowProc.
	// It isn't kept around between update and render, so attempting to save
	// state here that you want to carry over from one to the other will not work!!!
	// AKA, I messed up.
	struct WindowContext
	{
		WindowContext(struct Window *window, struct WindowStyle *windowStyle, bool hideWidgets, RenderBuffer *renderBuffer);

		Window *window;
		WindowStyle *windowStyle;

		bool hideWidgets;
		RenderBuffer *renderBuffer;

		Panel windowPanel;

		// Results
		bool closeRequested = false;
	};

	struct Window
	{
		String title;
		u32 flags;

		Rect2I area;
		String styleName;

		WindowProc windowProc;
		void *userData;
		WindowProc onClose;

		// Only used temporarily within updateAndRenderWindows()!
		// In user code, use WindowContext::renderBuffer instead
		RenderBuffer *renderBuffer;

		bool isInitialised;
	};

	// TODO: I don't like the API for this. Ideally we'd just say
	// "showWindow(proc)" and it would figure out its size and stuff on its own.
	// As it is, the user code has to know way too much about the window. We end
	// up creating showXWindow() functions to wrap the showWindow call for each
	// window type.
	void showWindow(String title, s32 width, s32 height, V2I position, String styleName, u32 flags, WindowProc windowProc, void *userData = null, WindowProc onClose = null);

	bool hasPauseWindowOpen();
	bool isWindowOpen(WindowProc windowProc);

	// Close any open windows that use the given WindowProc.
	// Generally, there will only be one window per WindowProc, so that's an easy 1-to-1 mapping.
	// If we later want better granularity, I'll have to figure something else out!
	void closeWindow(WindowProc windowProc);
	void closeAllWindows();

	void updateAndRenderWindows();

	//
	// INTERNAL
	//
	static Rect2I getWindowContentArea(Rect2I windowArea, s32 barHeight, s32 contentPadding);
	static void closeWindow(s32 windowIndex);
}
