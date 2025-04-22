#pragma once

class idUserInterfaceLocal;
class idRenderWindow : public idWindow
{
public:
	idRenderWindow( idUserInterfaceLocal* gui );
	virtual ~idRenderWindow();

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

private:
	void CommonInit();
	virtual bool ParseInternalVar( const char* name, idTokenParser* src );
	void Render( int time );
	void PreRender();
	void BuildAnimation( int time );
	renderView_t refdef;
	idRenderWorld* world;
	renderEntity_t worldEntity;
	renderLight_t rLight;
	const idMD5Anim* modelAnim;

	qhandle_t	worldModelDef;
	qhandle_t	lightDef;
	qhandle_t   modelDef;
	idWinStr modelName;
	idWinStr animName;
	idStr	 animClass;
	idWinVec4 lightOrigin;
	idWinVec4 lightColor;
	idWinVec4 modelOrigin;
	idWinVec4 modelRotate;
	idWinVec4 viewOffset;
	idWinBool needsRender;
	int animLength;
	int animEndTime;
	bool updateAnimation;
};
