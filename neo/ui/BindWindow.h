#pragma once

class idUserInterfaceLocal;
class idBindWindow : public idWindow
{
public:
	idBindWindow( idUserInterfaceLocal* gui );
	virtual ~idBindWindow();

	virtual const char* HandleEvent( const sysEvent_t* event, bool* updateVisuals );
	virtual void PostParse();
	virtual void Draw( int time, float x, float y );
	virtual size_t Allocated()
	{
		return idWindow::Allocated();
	};
//
//
	virtual idWinVar* GetWinVarByName( const char* _name, bool winLookup = false, drawWin_t** owner = NULL );
//
	virtual void Activate( bool activate, idStr& act );

private:
	void CommonInit();
	idWinStr bindName;
	bool waitingOnKey;
};