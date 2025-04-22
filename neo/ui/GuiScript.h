#pragma once 

#include "Window.h"
#include "Winvar.h"

struct idGSWinVar
{
	idGSWinVar()
	{
		var = NULL;
		own = false;
	}
	idWinVar* var;
	bool own;
};

class idGuiScriptList;

class idGuiScript
{
	friend class idGuiScriptList;
	friend class idWindow;

public:
	idGuiScript();
	~idGuiScript();

	bool Parse( idTokenParser* src );
	void Execute( idWindow* win )
	{
		if( handler )
		{
			handler( win, &parms );
		}
	}
	void FixupParms( idWindow* win );
	size_t Size()
	{
		int sz = sizeof( *this );
		for( int i = 0; i < parms.Num(); i++ )
		{
			sz += parms[i].var->Size();
		}
		return sz;
	}

	void WriteToSaveGame( idFile* savefile );
	void ReadFromSaveGame( idFile* savefile );

protected:
	int conditionReg;
	idGuiScriptList* ifList;
	idGuiScriptList* elseList;
	idList<idGSWinVar, TAG_OLD_UI> parms;
	void ( *handler )( idWindow* window, idList<idGSWinVar, TAG_OLD_UI>* src );

};


class idGuiScriptList
{
	idList<idGuiScript*, TAG_OLD_UI> list;
public:
	idGuiScriptList()
	{
		list.SetGranularity( 4 );
	};
	~idGuiScriptList()
	{
		list.DeleteContents( true );
	};
	void Execute( idWindow* win );
	void Append( idGuiScript* gs )
	{
		list.Append( gs );
	}
	size_t Size()
	{
		int sz = sizeof( *this );
		for( int i = 0; i < list.Num(); i++ )
		{
			sz += list[i]->Size();
		}
		return sz;
	}
	void FixupParms( idWindow* win );

	void WriteToSaveGame( idFile* savefile );
	void ReadFromSaveGame( idFile* savefile );
};