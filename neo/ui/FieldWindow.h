#pragma once

#include "Window.h"

class idFieldWindow : public idWindow
{
public:
	idFieldWindow( idUserInterfaceLocal* gui );
	virtual ~idFieldWindow();

	virtual void Draw( int time, float x, float y );

private:
	virtual bool ParseInternalVar( const char* name, idTokenParser* src );
	void CommonInit();
	void CalcPaintOffset( int len );
	int cursorPos;
	int lastTextLength;
	int lastCursorPos;
	int paintOffset;
	bool showCursor;
	idStr cursorVar;
};