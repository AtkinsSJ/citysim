// immediate-ui.cpp

void uiLabel(GLRenderer *renderer, BitmapFont *font, char *text, V2 origin, int32 align, V4 color)
{
	// TODO: Implement alignment!
	V2 topLeft = origin; //calculateTextPosition(label->cache, label->origin, label->align);
	
	drawText(renderer, font, topLeft, text, color);
}