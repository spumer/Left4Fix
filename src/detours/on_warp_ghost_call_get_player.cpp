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

#include "extension.h"
#include "on_warp_ghost_call_get_player.h"
 
namespace Detours
{
	CBaseEntity* OnWarpGhostCallGetPlayer::GetPlayerByCharacterDetour(int survivorType) {
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

			pPlayer = pCurPlayer;
			if(skip-- <= 0) {
				L4D_DEBUG_LOG("GetPlayerByCharacterDetour was found: name=%s", pInfo->GetName());
				break;	
			}
		}

		if(!pPlayer) {
			// this a static function detour -- no "this" pointer
			pPlayer = (GetTrampoline())(survivorType);
		}
		return pPlayer;
	}

	unsigned char* OnWarpGhostCallGetPlayer::GetSignatureAddress() {
		int offset;
		unsigned char* pGetPlayerCall;

		g_pGameConf->GetMemSig("WarpGhost_GetPlayerByCharacter", (void **)&pGetPlayerCall);
		if( !pGetPlayerCall ) {
			g_pSM->LogError(myself, "Can't get the \"WarpCode\" signature: %x", pGetPlayerCall);
			return nullptr;
		}

		if(g_pGameConf->GetOffset("WarpGhost_GetPlayerByCharacter", &offset)) {
			pGetPlayerCall += offset;
		}

		return pGetPlayerCall;
	}
}