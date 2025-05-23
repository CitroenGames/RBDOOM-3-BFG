/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2024 Robert Beckebans

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "precompiled.h"


#if defined(USE_INTRINSICS_SSE)
	#if MOC_MULTITHREADED
		#include "CullingThreadPool.h"
	#else
		#include "../libs/moc/MaskedOcclusionCulling.h"
	#endif
#endif

#include "RenderCommon.h"

extern idCVar r_useAreasConnectedForShadowCulling;
extern idCVar r_useParallelAddShadows;
extern idCVar r_forceShadowCaps;
extern idCVar r_useShadowPreciseInsideTest;

idCVar r_useAreasConnectedForShadowCulling( "r_useAreasConnectedForShadowCulling", "2", CVAR_RENDERER | CVAR_INTEGER, "cull entities cut off by doors" );
idCVar r_useParallelAddLights( "r_useParallelAddLights", "1", CVAR_RENDERER | CVAR_BOOL | CVAR_NOCHEAT, "aadd all lights in parallel with jobs" );

/*
============================
R_ShadowBounds

Even though the extruded shadows are drawn projected to infinity, their effects are limited
to a fraction of the light's volume.  An extruded box would require 12 faces to specify and
be a lot of trouble, but an axial bounding box is quick and easy to determine.

If the light is completely contained in the view, there is no value in trying to cull the
shadows, as they will all pass.

Pure function.
============================
*/
void R_ShadowBounds( const idBounds& modelBounds, const idBounds& lightBounds, const idVec3& lightOrigin, idBounds& shadowBounds )
{
	for( int i = 0; i < 3; i++ )
	{
		shadowBounds[0][i] = __fsels( modelBounds[0][i] - lightOrigin[i], modelBounds[0][i], lightBounds[0][i] );
		shadowBounds[1][i] = __fsels( lightOrigin[i] - modelBounds[1][i], modelBounds[1][i], lightBounds[1][i] );
	}
}

/*
============================
idRenderEntityLocal::IsDirectlyVisible()
============================
*/
bool idRenderEntityLocal::IsDirectlyVisible() const
{
	if( viewCount != tr.viewCount )
	{
		return false;
	}
	if( viewEntity->scissorRect.IsEmpty() )
	{
		// a viewEntity was created for shadow generation, but the
		// model global reference bounds isn't directly visible
		return false;
	}
	return true;
}

/*
===================
R_AddSingleLight

May be run in parallel.

Sets vLight->removeFromList to true if the light should be removed from the list.
Builds a chain of entities that need to be added for shadows only off vLight->shadowOnlyViewEntities.
Allocates and fills in vLight->entityInteractionState.

Calc the light shader values, removing any light from the viewLight list
if it is determined to not have any visible effect due to being flashed off or turned off.

Add any precomputed shadow volumes.
===================
*/
static void R_AddSingleLight( viewLight_t* vLight )
{
	// until proven otherwise
	vLight->removeFromList = true;
	vLight->shadowOnlyViewEntities = NULL;

	// globals we really should pass in...
	const viewDef_t* viewDef = tr.viewDef;

	const idRenderLightLocal* light = vLight->lightDef;
	const idMaterial* lightShader = light->lightShader;
	if( lightShader == NULL )
	{
		common->Error( "R_AddSingleLight: NULL lightShader" );
		return;
	}

	SCOPED_PROFILE_EVENT( lightShader->GetName() );

	// see if we are suppressing the light in this view
	if( !r_skipSuppress.GetBool() )
	{
		if( light->parms.suppressLightInViewID && light->parms.suppressLightInViewID == viewDef->renderView.viewID )
		{
			return;
		}
		if( light->parms.allowLightInViewID && light->parms.allowLightInViewID != viewDef->renderView.viewID )
		{
			return;
		}
	}

	// evaluate the light shader registers
	float* lightRegs = ( float* )R_FrameAlloc( lightShader->GetNumRegisters() * sizeof( float ), FRAME_ALLOC_SHADER_REGISTER );
	lightShader->EvaluateRegisters( lightRegs, light->parms.shaderParms, viewDef->renderView.shaderParms,
									tr.viewDef->renderView.time[0] * 0.001f, light->parms.referenceSound );

	// if this is a purely additive light and no stage in the light shader evaluates
	// to a positive light value, we can completely skip the light
	if( !lightShader->IsFogLight() && !lightShader->IsBlendLight() )
	{
		int lightStageNum;
		for( lightStageNum = 0; lightStageNum < lightShader->GetNumStages(); lightStageNum++ )
		{
			const shaderStage_t*	lightStage = lightShader->GetStage( lightStageNum );

			// ignore stages that fail the condition
			if( !lightRegs[ lightStage->conditionRegister ] )
			{
				continue;
			}

			const int* registers = lightStage->color.registers;

			// snap tiny values to zero
			if( lightRegs[ registers[0] ] < 0.001f )
			{
				lightRegs[ registers[0] ] = 0.0f;
			}
			if( lightRegs[ registers[1] ] < 0.001f )
			{
				lightRegs[ registers[1] ] = 0.0f;
			}
			if( lightRegs[ registers[2] ] < 0.001f )
			{
				lightRegs[ registers[2] ] = 0.0f;
			}

			if( lightRegs[ registers[0] ] > 0.0f ||
					lightRegs[ registers[1] ] > 0.0f ||
					lightRegs[ registers[2] ] > 0.0f )
			{
				break;
			}
		}

		if( lightStageNum == lightShader->GetNumStages() )
		{
			// we went through all the stages and didn't find one that adds anything
			// remove the light from the viewLights list, and change its frame marker
			// so interaction generation doesn't think the light is visible and
			// create a shadow for it
			return;
		}
	}


	//--------------------------------------------
	// copy data used by backend
	//--------------------------------------------
	vLight->globalLightOrigin = light->globalLightOrigin;
	vLight->lightProject[0] = light->lightProject[0];
	vLight->lightProject[1] = light->lightProject[1];
	vLight->lightProject[2] = light->lightProject[2];
	vLight->lightProject[3] = light->lightProject[3];

	// the fog plane is the light far clip plane
	idPlane fogPlane(	light->baseLightProject[2][0] - light->baseLightProject[3][0],
						light->baseLightProject[2][1] - light->baseLightProject[3][1],
						light->baseLightProject[2][2] - light->baseLightProject[3][2],
						light->baseLightProject[2][3] - light->baseLightProject[3][3] );
	const float planeScale = idMath::InvSqrt( fogPlane.Normal().LengthSqr() );
	vLight->fogPlane[0] = fogPlane[0] * planeScale;
	vLight->fogPlane[1] = fogPlane[1] * planeScale;
	vLight->fogPlane[2] = fogPlane[2] * planeScale;
	vLight->fogPlane[3] = fogPlane[3] * planeScale;

	// copy the matrix for deforming the 'zeroOneCubeModel' to exactly cover the light volume in world space
	vLight->inverseBaseLightProject = light->inverseBaseLightProject;

	// RB begin
	vLight->baseLightProject = light->baseLightProject;
	vLight->pointLight = light->parms.pointLight;
	vLight->parallel = light->parms.parallel;
	vLight->lightCenter = light->parms.lightCenter;
	// RB end

	vLight->falloffImage = light->falloffImage;
	vLight->lightShader = light->lightShader;
	vLight->shaderRegisters = lightRegs;

	const bool lightCastsShadows = light->LightCastsShadows();

	if( r_useLightScissors.GetInteger() != 0 )
	{
		// Calculate the matrix that projects the zero-to-one cube to exactly cover the
		// light frustum in clip space.
		idRenderMatrix invProjectMVPMatrix;
		idRenderMatrix::Multiply( viewDef->worldSpace.mvp, light->inverseBaseLightProject, invProjectMVPMatrix );

		// Calculate the projected bounds, either not clipped at all, near clipped, or fully clipped.
		idBounds projected;
		if( r_useLightScissors.GetInteger() == 1 )
		{
			idRenderMatrix::ProjectedBounds( projected, invProjectMVPMatrix, bounds_zeroOneCube );
		}
		else if( r_useLightScissors.GetInteger() == 2 )
		{
			idRenderMatrix::ProjectedNearClippedBounds( projected, invProjectMVPMatrix, bounds_zeroOneCube );
		}
		else
		{
			idRenderMatrix::ProjectedFullyClippedBounds( projected, invProjectMVPMatrix, bounds_zeroOneCube );
		}

		if( projected[0][2] >= projected[1][2] )
		{
			// the light was culled to the view frustum
			return;
		}

		float screenWidth = ( float )viewDef->viewport.x2 - ( float )viewDef->viewport.x1;
		float screenHeight = ( float )viewDef->viewport.y2 - ( float )viewDef->viewport.y1;

		idScreenRect lightScissorRect;
		lightScissorRect.x1 = idMath::Ftoi( projected[0][0] * screenWidth );
		lightScissorRect.x2 = idMath::Ftoi( projected[1][0] * screenWidth );
		lightScissorRect.y1 = idMath::Ftoi( projected[0][1] * screenHeight );
		lightScissorRect.y2 = idMath::Ftoi( projected[1][1] * screenHeight );
		lightScissorRect.Expand();

		vLight->scissorRect.Intersect( lightScissorRect );
		vLight->scissorRect.zmin = projected[0][2];
		vLight->scissorRect.zmax = projected[1][2];

		const bool viewInsideLight = !idRenderMatrix::CullPointToMVP( light->baseLightProject, viewDef->renderView.vieworg, true );

		// RB: test surface visibility by drawing the triangles of the bounds
#if defined(USE_INTRINSICS_SSE)

		if( r_useMaskedOcclusionCulling.GetBool() && !viewInsideLight && !viewDef->isMirror && !viewDef->isSubview )
		{
			idVec4 triVerts[8];
			unsigned int triIndices[] = { 0, 1, 2 };

			tr.pc.c_mocIndexes += 36;
			tr.pc.c_mocVerts += 8;

			idRenderMatrix invProjectMVPMatrix;

			// draw light volume 1 percentage bigger to avoid flickering
			// right before entering the volume with the camera
			const float mocLightScale = 0.99f;
			idRenderMatrix scaledInverseBaseLightProject = light->inverseBaseLightProject;
			scaledInverseBaseLightProject[0][0] *= mocLightScale;
			scaledInverseBaseLightProject[0][1] *= mocLightScale;
			scaledInverseBaseLightProject[0][2] *= mocLightScale;

			scaledInverseBaseLightProject[1][0] *= mocLightScale;
			scaledInverseBaseLightProject[1][1] *= mocLightScale;
			scaledInverseBaseLightProject[1][2] *= mocLightScale;

			scaledInverseBaseLightProject[2][0] *= mocLightScale;
			scaledInverseBaseLightProject[2][1] *= mocLightScale;
			scaledInverseBaseLightProject[2][2] *= mocLightScale;

			idRenderMatrix::Multiply( viewDef->worldSpace.unjitteredMVP, scaledInverseBaseLightProject, invProjectMVPMatrix );

			tr.pc.c_mocTests += 1;

			float wmin = idMath::INFINITUM;

			// NOTE: zeroToOne cube is only for lights and models need the unit cube
			idVec4* verts = tr.maskedZeroOneCubeVerts;
			for( int i = 0; i < 8; i++ )
			{
				// transform to clip space
				invProjectMVPMatrix.TransformPoint( verts[i], triVerts[i] );

				float w = triVerts[i].w;
				if( i == 0 )
				{
					wmin = w;
				}
				else if( w < wmin )
				{
					wmin = w;
				}
			}

			if( vLight->pointLight || vLight->parallel )
			{
#if 1
				// backface none so objects are still visible where we run into
#if MOC_MULTITHREADED
				tr.maskedOcclusionThreaded->SetMatrix( NULL );
				MaskedOcclusionCulling::CullingResult result = tr.maskedOcclusionThreaded->TestTriangles( ( float* )triVerts, tr.maskedZeroOneCubeIndexes, 12, MaskedOcclusionCulling::BACKFACE_NONE );
#else
				MaskedOcclusionCulling::CullingResult result = tr.maskedOcclusionCulling->TestTriangles( ( float* )triVerts, tr.maskedZeroOneCubeIndexes, 12, NULL, MaskedOcclusionCulling::BACKFACE_NONE );
#endif
				if( result != MaskedOcclusionCulling::VISIBLE )
				{
					tr.pc.c_mocCulledLights += 1;
					return;
				}
#else
				// draw for debugging
				tr.maskedOcclusionCulling->RenderTriangles( ( float* )triVerts, triIndices, 1, NULL, MaskedOcclusionCulling::BACKFACE_NONE );
				maskVisible = true;
#endif
			}
			else
			{
				// scissor test alternative

				// source scissor rectangle has GL convention and starts in the lower left corner
				// convert to NDC values
				float x1 = -1.0f + ( float( vLight->scissorRect.x1 ) / screenWidth ) * 2.0f;
				float x2 = -1.0f + ( float( vLight->scissorRect.x2 ) / screenWidth ) * 2.0f;
				float y1 = -1.0f + ( float( vLight->scissorRect.y1 ) / screenHeight ) * 2.0f;
				float y2 = -1.0f + ( float( vLight->scissorRect.y2 ) / screenHeight ) * 2.0f;

				float zmin = vLight->scissorRect.zmin;
				//zmin = 2.0f * zmin -1.0f;
				zmin = 1.0 - zmin; // reverse depth
				float wmin2 = ( 1.0 / zmin );
				wmin2 *= wmin;
				wmin2 = Max( wmin2, 0.0f );

				MaskedOcclusionCulling::CullingResult result = tr.maskedOcclusionCulling->TestRect( x1, y1, x2, y2, wmin2 );
				if( result != MaskedOcclusionCulling::VISIBLE )
				{
					tr.pc.c_mocCulledLights += 1;
					return;
				}
			}
		}
#endif
		// RB end

		// RB: calculate shadow LOD similar to Q3A .md3 LOD code

		// -1 means no shadows
		vLight->shadowLOD = -1;
		vLight->shadowFadeOut = 0;

		if( lightCastsShadows )
		{
			float           flod, lodscale;
			float           projectedRadius;
			int             lod;
			int             numLods;

			vLight->shadowLOD = 0;

			numLods = MAX_SHADOWMAP_RESOLUTIONS;

			// compute projected bounding sphere
			// and use that as a criteria for selecting LOD
			idVec3 center = projected.GetCenter();
			projectedRadius = projected.GetRadius( center );
			if( projectedRadius > 1.0f )
			{
				projectedRadius = 1.0f;
			}

			if( projectedRadius != 0 )
			{
				lodscale = r_shadowMapLodScale.GetFloat();

				if( lodscale > 20 )
				{
					lodscale = 20;
				}

				flod = 1.0f - projectedRadius * lodscale;
			}
			else
			{
				// object intersects near view plane, e.g. view weapon
				flod = 0;
			}

			// +1 allow to be so distant so we turn off shadow mapping
			flod *= ( numLods + 1 );

			if( flod < 0 )
			{
				flod = 0;
			}

			lod = idMath::Ftoi( flod );
			lod += r_shadowMapLodBias.GetInteger();

			if( lod < 0 )
			{
				lod = 0;
			}

			if( lod >= numLods )
			{
				// don't draw any shadow
				lod = -1;
			}

			if( lod == ( numLods - 1 ) )
			{
				// blend shadows smoothly in
				vLight->shadowFadeOut = idMath::Frac( flod );
			}

			// 2048^2 ultra quality is only for cascaded shadow mapping with sun lights
			if( lod == 0 && !light->parms.parallel )
			{
				lod = 1;
			}

			vLight->shadowLOD = lod;
		}
		// RB end
	}

	// this one stays on the list
	vLight->removeFromList = false;

	//--------------------------------------------
	// create interactions with all entities the light may touch, and add viewEntities
	// that may cast shadows, even if they aren't directly visible.  Any real work
	// will be deferred until we walk through the viewEntities
	//--------------------------------------------
	const int renderViewID = viewDef->renderView.viewID;

	// this bool array will be set true whenever the entity will visibly interact with the light
	vLight->entityInteractionState = ( byte* )R_ClearedFrameAlloc( light->world->entityDefs.Num() * sizeof( vLight->entityInteractionState[0] ), FRAME_ALLOC_INTERACTION_STATE );

	idInteraction** const interactionTableRow = light->world->interactionTable + light->index * light->world->interactionTableWidth;

	for( areaReference_t* lref = light->references; lref != NULL; lref = lref->ownerNext )
	{
		portalArea_t* area = lref->area;

		// some lights have their center of projection outside the world, but otherwise
		// we want to ignore areas that are not connected to the light center due to a closed door
		if( light->areaNum != -1 && r_useAreasConnectedForShadowCulling.GetInteger() == 2 )
		{
			if( !light->world->AreasAreConnected( light->areaNum, area->areaNum, PS_BLOCK_VIEW ) )
			{
				// can't possibly be seen or shadowed
				continue;
			}
		}

		// check all the models in this area
		for( areaReference_t* eref = area->entityRefs.areaNext; eref != &area->entityRefs; eref = eref->areaNext )
		{
			idRenderEntityLocal* edef = eref->entity;

			if( vLight->entityInteractionState[ edef->index ] != viewLight_t::INTERACTION_UNCHECKED )
			{
				continue;
			}
			// until proven otherwise
			vLight->entityInteractionState[ edef->index ] = viewLight_t::INTERACTION_NO;

			// The table is updated at interaction::AllocAndLink() and interaction::UnlinkAndFree()

			// TODO(Stephen): interactionTableRow is null if renderDef is used in a gui.sub
			const idInteraction* inter = interactionTableRow[ edef->index ];

			const renderEntity_t& eParms = edef->parms;
			const idRenderModel* eModel = eParms.hModel;

			// a large fraction of static entity / light pairs will still have no interactions even though
			// they are both present in the same area(s)
			if( eModel != NULL && !eModel->IsDynamicModel() && inter == INTERACTION_EMPTY )
			{
				// the interaction was statically checked, and it didn't generate any surfaces,
				// so there is no need to force the entity onto the view list if it isn't
				// already there
				continue;
			}

			// We don't want the lights on weapons to illuminate anything else.
			// There are two assumptions here -- that allowLightInViewID is only
			// used for weapon lights, and that all weapons will have weaponDepthHack.
			// A more general solution would be to have an allowLightOnEntityID field.
			// HACK: the armor-mounted flashlight is a private spot light, which is probably
			// wrong -- you would expect to see them in multiplayer.
			//	if( light->parms.allowLightInViewID && light->parms.pointLight && !eParms.weaponDepthHack )
			//	{
			//		continue;
			//	}

			// non-shadow casting entities don't need to be added if they aren't
			// directly visible
			if( ( eParms.noShadow || ( eModel && !eModel->ModelHasShadowCastingSurfaces() ) ) && !edef->IsDirectlyVisible() )
			{
				continue;
			}

			// if the model doesn't accept lighting or cast shadows, it doesn't need to be added
			if( eModel && !eModel->ModelHasInteractingSurfaces() && !eModel->ModelHasShadowCastingSurfaces() )
			{
				continue;
			}

			// no interaction present, so either the light or entity has moved
			// assert( lightHasMoved || edef->entityHasMoved );
			if( inter == NULL )
			{
				// some big outdoor meshes are flagged to not create any dynamic interactions
				// when the level designer knows that nearby moving lights shouldn't actually hit them
				if( eParms.noDynamicInteractions )
				{
					continue;
				}

				// do a check of the entity reference bounds against the light frustum to see if they can't
				// possibly interact, despite sharing one or more world areas
				if( R_CullModelBoundsToLight( light, edef->localReferenceBounds, edef->modelRenderMatrix ) )
				{
					continue;
				}
			}

			// we now know that the entity and light do overlap

			if( edef->IsDirectlyVisible() )
			{
				// entity is directly visible, so the interaction is definitely needed
				vLight->entityInteractionState[ edef->index ] = viewLight_t::INTERACTION_YES;
				continue;
			}

			// the entity is not directly visible, but if we can tell that it may cast
			// shadows onto visible surfaces, we must make a viewEntity for it
			if( !lightCastsShadows )
			{
				// surfaces are never shadowed in this light
				continue;
			}
			// if we are suppressing its shadow in this view (player shadows, etc), skip
			if( !r_skipSuppress.GetBool() )
			{
				if( eParms.suppressShadowInViewID && eParms.suppressShadowInViewID == renderViewID )
				{
					continue;
				}
				if( eParms.suppressShadowInLightID && eParms.suppressShadowInLightID == light->parms.lightId )
				{
					continue;
				}
			}

			// should we use the shadow bounds from pre-calculated interactions?
			idBounds shadowBounds;
			R_ShadowBounds( edef->globalReferenceBounds, light->globalLightBounds, light->globalLightOrigin, shadowBounds );

			// this test is pointless if we knew the light was completely contained
			// in the view frustum, but the entity would also be directly visible in most
			// of those cases.

			// this doesn't say that the shadow can't effect anything, only that it can't
			// effect anything in the view, so we shouldn't set up a view entity
			if( idRenderMatrix::CullBoundsToMVP( viewDef->worldSpace.mvp, shadowBounds ) )
			{
				continue;
			}

			// debug tool to allow viewing of only one entity at a time
			if( r_singleEntity.GetInteger() >= 0 && r_singleEntity.GetInteger() != edef->index )
			{
				continue;
			}

			// we do need it for shadows
			vLight->entityInteractionState[ edef->index ] = viewLight_t::INTERACTION_YES;

			// we will need to create a viewEntity_t for it in the serial code section
			shadowOnlyEntity_t* shadEnt = ( shadowOnlyEntity_t* )R_FrameAlloc( sizeof( shadowOnlyEntity_t ), FRAME_ALLOC_SHADOW_ONLY_ENTITY );
			shadEnt->next = vLight->shadowOnlyViewEntities;
			shadEnt->edef = edef;
			vLight->shadowOnlyViewEntities = shadEnt;
		}
	}
}

REGISTER_PARALLEL_JOB( R_AddSingleLight, "R_AddSingleLight" );

/*
=================
R_AddLights
=================
*/
void R_AddLights()
{
	SCOPED_PROFILE_EVENT( "R_AddLights" );

	//-------------------------------------------------
	// check each light individually, possibly in parallel
	//-------------------------------------------------

	if( r_useParallelAddLights.GetBool() )
	{
		for( viewLight_t* vLight = tr.viewDef->viewLights; vLight != NULL; vLight = vLight->next )
		{
			tr.frontEndJobList->AddJob( ( jobRun_t )R_AddSingleLight, vLight );
		}
		tr.frontEndJobList->Submit();
		tr.frontEndJobList->Wait();
	}
	else
	{
		for( viewLight_t* vLight = tr.viewDef->viewLights; vLight != NULL; vLight = vLight->next )
		{
			R_AddSingleLight( vLight );
		}
	}

	//-------------------------------------------------
	// cull lights from the list if they turned out to not be needed
	//-------------------------------------------------

	tr.pc.c_viewLights = 0;
	viewLight_t** ptr = &tr.viewDef->viewLights;
	while( *ptr != NULL )
	{
		viewLight_t* vLight = *ptr;

		if( vLight->removeFromList )
		{
			vLight->lightDef->viewCount = -1;	// this probably doesn't matter with current code
			*ptr = vLight->next;
			continue;
		}

		ptr = &vLight->next;

		// serial work
		tr.pc.c_viewLights++;

		for( shadowOnlyEntity_t* shadEnt = vLight->shadowOnlyViewEntities; shadEnt != NULL; shadEnt = shadEnt->next )
		{
			// this will add it to the viewEntities list, but with an empty scissor rect
			R_SetEntityDefViewEntity( shadEnt->edef );
		}

		if( r_showLightScissors.GetBool() )
		{
			R_ShowColoredScreenRect( vLight->scissorRect, vLight->lightDef->index );
		}
	}
}

/*
=====================
R_OptimizeViewLightsList
=====================
*/
void R_OptimizeViewLightsList()
{
	// go through each visible light
	int numViewLights = 0;
	for( viewLight_t* vLight = tr.viewDef->viewLights; vLight != NULL; vLight = vLight->next )
	{
		numViewLights++;
		// If the light didn't have any lit surfaces visible, there is no need to
		// draw any of the shadows.  We still keep the vLight for debugging draws.
		if( !vLight->localInteractions && !vLight->globalInteractions && !vLight->translucentInteractions )
		{
			vLight->localShadows = NULL;
			vLight->globalShadows = NULL;
		}
	}

	if( r_useShadowSurfaceScissor.GetBool() )
	{
		// shrink the light scissor rect to only intersect the surfaces that will actually be drawn.
		// This doesn't seem to actually help, perhaps because the surface scissor
		// rects aren't actually the surface, but only the portal clippings.
		for( viewLight_t* vLight = tr.viewDef->viewLights; vLight; vLight = vLight->next )
		{
			drawSurf_t* surf;
			idScreenRect surfRect;

			if( !vLight->lightShader->LightCastsShadows() )
			{
				continue;
			}

			surfRect.Clear();

			for( surf = vLight->globalInteractions; surf != NULL; surf = surf->nextOnLight )
			{
				surfRect.Union( surf->scissorRect );
			}
			for( surf = vLight->localShadows; surf != NULL; surf = surf->nextOnLight )
			{
				surf->scissorRect.Intersect( surfRect );
			}

			for( surf = vLight->localInteractions; surf != NULL; surf = surf->nextOnLight )
			{
				surfRect.Union( surf->scissorRect );
			}
			for( surf = vLight->globalShadows; surf != NULL; surf = surf->nextOnLight )
			{
				surf->scissorRect.Intersect( surfRect );
			}

			for( surf = vLight->translucentInteractions; surf != NULL; surf = surf->nextOnLight )
			{
				surfRect.Union( surf->scissorRect );
			}

			vLight->scissorRect.Intersect( surfRect );
		}
	}

	// sort the viewLights list so the largest lights come first, which will reduce
	// the chance of GPU pipeline bubbles
	struct sortLight_t
	{
		viewLight_t* 	vLight;
		int				screenArea;
		static int sort( const void* a, const void* b )
		{
			return ( ( sortLight_t* )a )->screenArea - ( ( sortLight_t* )b )->screenArea;
		}
	};
	sortLight_t* sortLights = ( sortLight_t* )_alloca( sizeof( sortLight_t ) * numViewLights );
	int	numSortLightsFilled = 0;
	for( viewLight_t* vLight = tr.viewDef->viewLights; vLight != NULL; vLight = vLight->next )
	{
		sortLights[ numSortLightsFilled ].vLight = vLight;
		sortLights[ numSortLightsFilled ].screenArea = vLight->scissorRect.GetArea();
		numSortLightsFilled++;
	}

	qsort( sortLights, numSortLightsFilled, sizeof( sortLights[0] ), sortLight_t::sort );

	// rebuild the linked list in order
	tr.viewDef->viewLights = NULL;
	for( int i = 0; i < numSortLightsFilled; i++ )
	{
		sortLights[i].vLight->next = tr.viewDef->viewLights;
		tr.viewDef->viewLights = sortLights[i].vLight;
	}
}
