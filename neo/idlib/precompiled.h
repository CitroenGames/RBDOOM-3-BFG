#pragma once 

#include "sys/sys_defines.h"
#include "sys/sys_builddefines.h"
#include "sys/sys_includes.h"
#include "sys/sys_assert.h"
#include "sys/sys_types.h"
#include "sys/sys_intrinsics.h"
#include "sys/sys_threading.h"

//-----------------------------------------------------

#define ID_TIME_T int64 // Signed because -1 means "File not found" and we don't want that to compare > than any other time

// non-portable system services
#include "../sys/sys_public.h"

// id lib
#include "../idlib/Lib.h"
#include "../idlib/gltfProperties.h"
#include "../idlib/gltfParser.h"


#include "sys/sys_filesystem.h"

// framework
#include "../framework/BuildVersion.h"
#include "../framework/Licensee.h"
#include "../framework/CmdSystem.h"
#include "../framework/CVarSystem.h"
#include "../framework/Common.h"
// DG: needed for idFile_InZip in File.h
#include "../framework/Unzip.h"
// DG end
#include "../framework/File.h"
#include "../framework/File_Manifest.h"
#include "../framework/File_SaveGame.h"
#include "../framework/File_Resource.h"
#include "../framework/File_Zip.h"
#include "../framework/FileSystem.h"
#include "../framework/UsercmdGen.h"
#include "../framework/Serializer.h"
#include "../framework/PlayerProfile.h"

// decls
#include "../framework/TokenParser.h"
#include "../framework/DeclManager.h"
#include "../framework/DeclTable.h"
#include "../framework/DeclSkin.h"
#include "../framework/DeclEntityDef.h"
#include "../framework/DeclFX.h"
#include "../framework/DeclParticle.h"
#include "../framework/DeclAF.h"
#include "../framework/DeclPDA.h"

// We have expression parsing and evaluation code in multiple places:
// materials, sound shaders, and guis. We should unify them.
const int MAX_EXPRESSION_OPS = 4096;
const int MAX_EXPRESSION_REGISTERS = 4096;

// renderer
// everything that is needed by the backend needs
// to be double buffered to allow it to run in
// parallel on a dual cpu machine
// SRS - use triple buffering for NVRHI with command queue event query sync method
constexpr uint32 NUM_FRAME_DATA	= 3;

#include "nvrhi/nvrhi.h"

// RB: make Optick profiling available everywhere
#include "../libs/optick/optick.h"

#include "../renderer/Cinematic.h"
#include "../renderer/Material.h"
#include "../renderer/BufferObject.h"
#include "../renderer/VertexCache.h"
#include "../renderer/Model.h"
#include "../renderer/ModelManager.h"
#include "../renderer/RenderSystem.h"
#include "../renderer/RenderWorld.h"
#include "../renderer/BindingCache.h"
#include "../renderer/RenderCommon.h"

// sound engine
#include "../sound/sound.h"

// user interfaces
#include "../ui/ListGUI.h"
#include "../ui/UserInterface.h"

// RB: required for SWF extensions
//#include "rapidjson/document.h"

#include "../swf/SWF.h"

// collision detection system
#include "../cm/CollisionModel.h"

// AAS files and manager
#include "../aas/AASFile.h"
#include "../aas/AASFileManager.h"

// game
#include "../d3xp/Game.h"

// Session / Network
#include "../sys/LightweightCompression.h"
#include "../sys/Snapshot.h"
#include "../sys/PacketProcessor.h"
#include "../sys/SnapshotProcessor.h"

#include "../sys/sys_savegame.h"
#include "../sys/sys_session_savegames.h"
#include "../sys/sys_profile.h"
#include "../sys/sys_localuser.h"
#include "../sys/sys_signin.h"
#include "../sys/sys_stats_misc.h"
#include "../sys/sys_stats.h"
#include "../sys/sys_session.h"
#include "../sys/sys_achievements.h"

// tools

// The editor entry points are always declared, but may just be
// stubbed out on non-windows platforms.
#include "../imgui/ImGui_Hooks.h"
#include "../tools/edit_public.h"

#include "../tools/compilers/compiler_public.h"

//-----------------------------------------------------

#ifndef _D3SDK

	#ifdef GAME_DLL

		#include "../d3xp/Game_local.h"

	#else

		#include "../framework/DemoChecksum.h"

		// framework
		#include "../framework/Compressor.h"
		#include "../framework/EventLoop.h"
		#include "../framework/KeyInput.h"
		#include "../framework/EditField.h"
		#include "../framework/DebugGraph.h"
		#include "../framework/Console.h"
		#include "../framework/Common_dialog.h"

	#endif /* !GAME_DLL */

#endif /* !_D3SDK */

//-----------------------------------------------------

#undef min
#undef max
#include <algorithm>	// for min / max / swap