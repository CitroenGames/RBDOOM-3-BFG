#pragma once

class idUserInterfaceLocal;
class idDeviceContext;
class idSimpleWindow;

typedef struct
{
	idWindow* win;
	idSimpleWindow* simp;
} drawWin_t;

class idSimpleWindow
{
	friend class idWindow;
public:
	idSimpleWindow( idWindow* win );
	virtual			~idSimpleWindow();
	void			Redraw( float x, float y );
	void			StateChanged( bool redraw );

	idStr			name;

	idWinVar* 		GetWinVarByName( const char* _name );
	int				GetWinVarOffset( idWinVar* wv, drawWin_t* owner );
	size_t			Size();

	idWindow*		GetParent()
	{
		return mParent;
	}

	virtual void	WriteToSaveGame( idFile* savefile );
	virtual void	ReadFromSaveGame( idFile* savefile );

protected:
	void 			CalcClientRect( float xofs, float yofs );
	void 			SetupTransforms( float x, float y );
	void 			DrawBackground( const idRectangle& drawRect );
	void 			DrawBorderAndCaption( const idRectangle& drawRect );

	idUserInterfaceLocal* gui;
	int 			flags;
	idRectangle 	drawRect;			// overall rect
	idRectangle 	clientRect;			// client area
	idRectangle 	textRect;
	idVec2			origin;
	class idFont* 	font;
	float 			matScalex;
	float 			matScaley;
	float 			borderSize;
	int 			textAlign;
	float 			textAlignx;
	float 			textAligny;
	int				textShadow;

	idWinStr		text;
	idWinBool		visible;
	idWinRectangle	rect;				// overall rect
	idWinVec4		backColor;
	idWinVec4		matColor;
	idWinVec4		foreColor;
	idWinVec4		borderColor;
	idWinFloat		textScale;
	idWinFloat		rotate;
	idWinVec2		shear;
	idWinBackground	backGroundName;

	const idMaterial* background;

	idWindow* 		mParent;

	idWinBool	hideCursor;
};