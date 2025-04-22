#pragma once

/*
===============================================================================

	feed data to a listDef
	each item has an id and a display string

===============================================================================
*/

class idListGUILocal : protected idList<idStr, TAG_OLD_UI>, public idListGUI
{
public:
	idListGUILocal()
	{
		m_pGUI = NULL;
		m_water = 0;
		m_stateUpdates = true;
	}

	// idListGUI interface
	void				Config( idUserInterface* pGUI, const char* name )
	{
		m_pGUI = pGUI;
		m_name = name;
	}
	void				Add( int id, const idStr& s );
	// use the element count as index for the ids
	void				Push( const idStr& s );
	bool				Del( int id );
	void				Clear();
	int					Num()
	{
		return idList<idStr, TAG_OLD_UI>::Num();
	}
	int					GetSelection( char* s, int size, int sel = 0 ) const; // returns the id, not the list index (or -1)
	void				SetSelection( int sel );
	int					GetNumSelections();
	bool				IsConfigured() const;
	void				SetStateChanges( bool enable );
	void				Shutdown();

private:
	idUserInterface* 	m_pGUI;
	idStr				m_name;
	int					m_water;
	idList<int, TAG_OLD_UI>			m_ids;
	bool				m_stateUpdates;

	void				StateChanged();
};
