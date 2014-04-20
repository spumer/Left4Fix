/**
 * vim: set ts=4 :
 * =============================================================================
 * Left 4 Fix SourceMod Extension
 * Copyright (C) 2014 Spumer.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include "warp_code.h"
#include "extension.h"
#include "asm/asm.h"
#include "CDetour/detourhelpers.h"
#include "patch_utils.hpp"


class CBaseEntity;
CBaseEntity* (*CTerrorPlayer_GetPlayerByCharacter)(int);

CBaseEntity* GetPlayerByCharacterDetour(int survivorType) {
	L4D_DEBUG_LOG("GetPlayerByCharacterDetour called for %d", survivorType);
	
	CBaseEntity* pPlayer = nullptr;

	int maxplayers = playerhelpers->GetMaxClients();
	for(int client = 1, skip = survivorType; client < maxplayers; ++client) {
		CBaseEntity* pCurPlayer = gamehelpers->ReferenceToEntity(client);
		if(!pCurPlayer) continue;

		IGamePlayer* pGamePlayer = playerhelpers->GetGamePlayer(client);
		if(!pGamePlayer || !pGamePlayer->IsConnected()) continue;

		IPlayerInfo* pInfo = pGamePlayer->GetPlayerInfo();
		if(!pInfo || pInfo->GetTeamIndex() != 2 || pInfo->IsObserver()) continue;

		if(skip <= 0) {
			L4D_DEBUG_LOG("GetPlayerByCharacterDetour was end: name=%s", pInfo->GetName());
			pPlayer = pCurPlayer;
			break;	
		}
		--skip;

		if(!pPlayer) {
			pPlayer = pCurPlayer;
		}
	}
	return pPlayer;
	// return CTerrorPlayer_GetPlayerByCharacter(survivorType);
}


void WarpCode::Patch() {
	if(m_isPatched) return;
	
	int offset;
	uint8_t* pGetPlayerCall;
	uint8_t* pMaxPlayerCount;
	uint8_t* pFunc;

	g_pGameConf->GetMemSig("WarpGhost_GetPlayerByCharacter", (void **)&pFunc);
	if( !pFunc ) {
		g_pSM->LogError(myself, "Can't get the \"WarpCode\" signature: %x", pFunc);
		return;
	}

	pGetPlayerCall = pFunc;
	if(g_pGameConf->GetOffset("WarpGhost_GetPlayerByCharacter", &offset)) {
		pGetPlayerCall += offset;
	}

	pMaxPlayerCount = pFunc;
	if(g_pGameConf->GetOffset("WarpGhost_MaxPlayerCount", &offset)) {
		pMaxPlayerCount += offset;
	}

	// Get the original function
	g_pGameConf->GetMemSig("CTerrorPlayer_GetPlayerByCharacter", (void **)&pFunc);
	if( !pFunc ) {
		g_pSM->LogError(myself, "Can't get the \"CTerrorPlayer_GetPlayerByCharacter\" signature: %x", pFunc);
		return;
	}

	#ifdef DEBUG
	memDump(pMaxPlayerCount, 8);
	#endif

	SetMemPatchable(pMaxPlayerCount, 1);
	*pMaxPlayerCount = static_cast<uint8_t>(TEAM_SIZE - 1);

	#ifdef DEBUG
	memDump(pMaxPlayerCount, 8);
	#endif

	SetMemPatchable(pGetPlayerCall, OP_CALL_SIZE);
	CTerrorPlayer_GetPlayerByCharacter = (CBaseEntity* (*)(int))pFunc;
	replace_call_addr((void *)pGetPlayerCall, (void *)GetPlayerByCharacterDetour);

	m_isPatched = true;
}

void WarpCode::Unpatch() {
	if(!m_isPatched) return;
	
	// ISourcePawnEngine *sengine = g_pSM->GetScriptingEngine();
	// if(m_injectMarker) { copy_bytes(m_pMarkers, UpdateMarkersReached_orig, sizeof(UpdateMarkersReached_orig)); sengine->FreePageMemory(m_injectMarker); }
	// if(m_injectStats) { copy_bytes(m_pL4DStats, AddSurvivorStats_orig, sizeof(AddSurvivorStats_orig)); sengine->FreePageMemory(m_injectStats); }
	// if(m_injectCompl) { copy_bytes(m_pCompletion, GetVersusCompletion_orig, sizeof(GetVersusCompletion_orig)); sengine->FreePageMemory(m_injectCompl); }
	
	m_isPatched = false;
}