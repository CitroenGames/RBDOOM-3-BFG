#pragma once

class idTokenParser;
class idWindow;
class idWinVar;

class idRegister
{
public:
	idRegister();
	idRegister( const char* p, int t );

	enum REGTYPE { VEC4 = 0, FLOAT, BOOL, INT, STRING, VEC2, VEC3, RECTANGLE, NUMTYPES } ;
	static int REGCOUNT[NUMTYPES];

	bool				enabled;
	short				type;
	idStr				name;
	int					regCount;
	unsigned short		regs[4];
	idWinVar* 			var;

	void				SetToRegs( float* registers );
	void				GetFromRegs( float* registers );
	void				CopyRegs( idRegister* src );
	void				Enable( bool b )
	{
		enabled = b;
	}
	void				WriteToSaveGame( idFile* savefile );
	void				ReadFromSaveGame( idFile* savefile );
};

ID_INLINE idRegister::idRegister()
{
}

ID_INLINE idRegister::idRegister( const char* p, int t )
{
	name = p;
	type = t;
	assert( t >= 0 && t < NUMTYPES );
	regCount = REGCOUNT[t];
	enabled = ( type == STRING ) ? false : true;
	var = NULL;
};

ID_INLINE void idRegister::CopyRegs( idRegister* src )
{
	regs[0] = src->regs[0];
	regs[1] = src->regs[1];
	regs[2] = src->regs[2];
	regs[3] = src->regs[3];
}

class idRegisterList
{
public:

	idRegisterList();
	~idRegisterList();

	void				AddReg( const char* name, int type, idTokenParser* src, idWindow* win, idWinVar* var );
	void				AddReg( const char* name, int type, idVec4 data, idWindow* win, idWinVar* var );

	idRegister* 		FindReg( const char* name );
	void				SetToRegs( float* registers );
	void				GetFromRegs( float* registers );
	void				Reset();
	void				WriteToSaveGame( idFile* savefile );
	void				ReadFromSaveGame( idFile* savefile );

private:
	idList<idRegister*>	regs;
	idHashIndex			regHash;
};

ID_INLINE idRegisterList::idRegisterList()
{
	regs.SetGranularity( 4 );
	regHash.SetGranularity( 4 );
	regHash.Clear( 32, 4 );
}

ID_INLINE idRegisterList::~idRegisterList()
{
}
