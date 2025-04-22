#pragma once

#include "Window.h"

class idUserInterfaceLocal;
class idChoiceWindow : public idWindow
{
public:
	idChoiceWindow( idUserInterfaceLocal* gui );
	virtual				~idChoiceWindow();

	virtual const char*	HandleEvent( const sysEvent_t* event, bool* updateVisuals );
	virtual void 		PostParse();
	virtual void 		Draw( int time, float x, float y );
	virtual void		Activate( bool activate, idStr& act );
	virtual size_t		Allocated()
	{
		return idWindow::Allocated();
	};

	virtual idWinVar*	GetWinVarByName( const char* _name, bool winLookup = false, drawWin_t** owner = NULL );

	void				RunNamedEvent( const char* eventName );

private:
	virtual bool		ParseInternalVar( const char* name, idTokenParser* src );
	void				CommonInit();
	void				UpdateChoice();
	void				ValidateChoice();

	void				InitVars();
	// true: read the updated cvar from cvar system, gui from dict
	// false: write to the cvar system, to the gui dict
	// force == true overrides liveUpdate 0
	void				UpdateVars( bool read, bool force = false );

	void				UpdateChoicesAndVals();

	int					currentChoice;
	int					choiceType;
	idStr				latchedChoices;
	idWinStr			choicesStr;
	idStr				latchedVals;
	idWinStr			choiceVals;
	idStrList			choices;
	idStrList			values;

	idWinStr			guiStr;
	idWinStr			cvarStr;
	idCVar* 			cvar;
	idMultiWinVar		updateStr;

	idWinBool			liveUpdate;
	idWinStr			updateGroup;
};