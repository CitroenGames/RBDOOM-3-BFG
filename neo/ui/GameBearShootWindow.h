#pragma once

class idGameBearShootWindow;

class BSEntity
{
public:
	const idMaterial* 		material;
	idStr					materialName;
	float					width, height;
	bool					visible;

	idVec4					entColor;
	idVec2					position;
	float					rotation;
	float					rotationSpeed;
	idVec2					velocity;

	bool					fadeIn;
	bool					fadeOut;

	idGameBearShootWindow* 	game;

public:
	BSEntity( idGameBearShootWindow* _game );
	virtual				~BSEntity();

	virtual void		WriteToSaveGame( idFile* savefile );
	virtual void		ReadFromSaveGame( idFile* savefile, idGameBearShootWindow* _game );

	void				SetMaterial( const char* name );
	void				SetSize( float _width, float _height );
	void				SetVisible( bool isVisible );

	virtual void		Update( float timeslice );
	virtual void		Draw();

private:
};


class idGameBearShootWindow : public idWindow
{
public:
	idGameBearShootWindow( idUserInterfaceLocal* gui );
	~idGameBearShootWindow();

	virtual void		WriteToSaveGame( idFile* savefile );
	virtual void		ReadFromSaveGame( idFile* savefile );

	virtual const char*	HandleEvent( const sysEvent_t* event, bool* updateVisuals );
	virtual void		PostParse();
	virtual void		Draw( int time, float x, float y );
	virtual const char*	Activate( bool activate );
	virtual idWinVar* 	GetWinVarByName( const char* _name, bool winLookup = false, drawWin_t** owner = NULL );

private:
	void				CommonInit();
	void				ResetGameState();

	void				UpdateBear();
	void				UpdateHelicopter();
	void				UpdateTurret();
	void				UpdateButtons();
	void				UpdateGame();
	void				UpdateScore();

	virtual bool		ParseInternalVar( const char* name, idTokenParser* src );

private:

	idWinBool			gamerunning;
	idWinBool			onFire;
	idWinBool			onContinue;
	idWinBool			onNewGame;

	float				timeSlice;
	float				timeRemaining;
	bool				gameOver;

	int					currentLevel;
	int					goalsHit;
	bool				updateScore;
	bool				bearHitTarget;

	float				bearScale;
	bool				bearIsShrinking;
	int					bearShrinkStartTime;

	float				turretAngle;
	float				turretForce;

	float				windForce;
	int					windUpdateTime;

	idList<BSEntity*>	entities;

	BSEntity*			turret;
	BSEntity*			bear;
	BSEntity*			helicopter;
	BSEntity*			goal;
	BSEntity*			wind;
	BSEntity*			gunblast;
};
