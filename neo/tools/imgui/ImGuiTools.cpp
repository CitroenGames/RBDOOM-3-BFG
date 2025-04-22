#include "precompiled.h"


#include "../imgui/BFGimgui.h"
#include "../idlib/CmdArgs.h"

#include "afeditor/AfEditor.h"
#include "lighteditor/LightEditor.h"


extern idCVar g_editEntityMode;

static bool releaseMouse = false;


namespace ImGuiTools
{

void SetReleaseToolMouse( bool doRelease )
{
	releaseMouse = doRelease;
}

bool AreEditorsActive()
{
	// FIXME: this is not exactly clean and must be changed if we ever support game dlls
	return g_editEntityMode.GetInteger() > 0 || com_editors != 0;
}

bool ReleaseMouseForTools()
{
	// RB: ignore everything as long right mouse button is pressed
	return AreEditorsActive() && releaseMouse && !ImGuiHook::RightMouseActive();
}

void DrawToolWindows()
{
	if( !AreEditorsActive() )
	{
		return;
	}

	if( LightEditor::Instance().IsShown() )
	{
		LightEditor::Instance().Draw();
	}
	else if( AfEditor::Instance().IsShown() )
	{
		AfEditor::Instance().Draw();
	}
}

void LightEditorInit( const idDict* dict, idEntity* ent )
{
	if( dict == nullptr || ent == nullptr )
	{
		return;
	}

	// NOTE: we can't access idEntity (it's just a declaration), because it should
	// be game/mod specific. but we can at least check the spawnclass from the dict.
	idassert( idStr::Icmp( dict->GetString( "spawnclass" ), "idLight" ) == 0
			  && "LightEditorInit() must only be called with light entities or nullptr!" );


	LightEditor::Instance().ShowIt( true );
	SetReleaseToolMouse( true );

	LightEditor::ReInit( dict, ent );
}

void AfEditorInit()
{
	AfEditor::Instance().ShowIt( true );
	SetReleaseToolMouse( true );
}

} //namespace ImGuiTools
