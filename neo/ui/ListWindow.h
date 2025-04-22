#pragma once 

class idSliderWindow;

enum
{
	TAB_TYPE_TEXT = 0,
	TAB_TYPE_ICON = 1
};

struct idTabRect
{
	int x;
	int w;
	int align;
	int valign;
	int	type;
	idVec2 iconSize;
	float iconVOffset;
};

class idListWindow : public idWindow
{
public:
	idListWindow( idUserInterfaceLocal* gui );

	virtual const char*	HandleEvent( const sysEvent_t* event, bool* updateVisuals );
	virtual void		PostParse();
	virtual void		Draw( int time, float x, float y );
	virtual void		Activate( bool activate, idStr& act );
	virtual void		HandleBuddyUpdate( idWindow* buddy );
	virtual void		StateChanged( bool redraw = false );
	virtual size_t		Allocated()
	{
		return idWindow::Allocated();
	};
	virtual idWinVar*	GetWinVarByName( const char* _name, bool winLookup = false, drawWin_t** owner = NULL );

	void				UpdateList();

private:
	virtual bool		ParseInternalVar( const char* name, idTokenParser* src );
	void				CommonInit();
	void				InitScroller( bool horizontal );
	void				SetCurrentSel( int sel );
	void				AddCurrentSel( int sel );
	int					GetCurrentSel();
	bool				IsSelected( int index );
	void				ClearSelection( int sel );

	idList<idTabRect, TAG_OLD_UI>	tabInfo;
	int					top;
	float				sizeBias;
	bool				horizontal;
	idStr				tabStopStr;
	idStr				tabAlignStr;
	idStr				tabVAlignStr;
	idStr				tabTypeStr;
	idStr				tabIconSizeStr;
	idStr				tabIconVOffsetStr;
	idHashTable<const idMaterial*> iconMaterials;
	bool				multipleSel;

	idStrList			listItems;
	idSliderWindow*		scroller;
	idList<int, TAG_OLD_UI>			currentSel;
	idStr				listName;

	int					clickTime;

	int					typedTime;
	idStr				typed;
};
