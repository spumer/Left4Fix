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

#include <asm/asm.h>
#include "extension.h"

#include "CDetour/detourhelpers.h"
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
			if(!pInfo || pInfo->GetTeamIndex() != 2) continue;

			if(skip > 0) { skip -= 1; continue; }

			if(pInfo->IsObserver()) continue;

			pPlayer = pCurPlayer;

			L4D_DEBUG_LOG("GetPlayerByCharacterDetour was found: name=%s", pInfo->GetName());
			break;
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

		g_pGameConf->GetMemSig("CTerrorPlayer_WarpGhostToInitialPosition", (void **)&pGetPlayerCall);
		if( !pGetPlayerCall ) {
			g_pSM->LogError(myself, "Can't get the \"CTerrorPlayer_WarpGhostToInitialPosition\" signature: %x", pGetPlayerCall);
			return nullptr;
		}

		if( !g_pGameConf->GetOffset("WarpGhostToInitialPosition__GetPlayerByCharacter", &offset) || !offset ) {
			g_pSM->LogError(myself, "WarpGhost -- Could not find 'WarpGhostToInitialPosition__GetPlayerByCharacter' offset");
			return nullptr;
		}
		pGetPlayerCall += offset;

#ifdef WIN32
		// Windows version inline function body
		// NOP code and insert call instruction instead
		int need_nop;
		void *addr;
		uint8_t push_arg[] = { 0x50 };  // push eax
		uint8_t cleanup_stack[] = { 0x83, 0xC4, 0x04 };  // add esp, 4

		if( !g_pGameConf->GetMemSig("CTerrorPlayer_GetPlayerByCharacter", (void **)&addr) || !addr ) {
			g_pSM->LogError(myself, "WarpGhost -- Could not find 'CTerrorPlayer_GetPlayerByCharacter' address");
			return nullptr;
		}

		if( !g_pGameConf->GetOffset("WarpGhostToInitialPosition__GetPlayerByCharacter_inline_len", &need_nop) || !need_nop ) {
			g_pSM->LogError(myself, "WarpGhost -- Could not find 'WarpGhostToInitialPosition__GetPlayerByCharacter_inline_len' offset");
			return nullptr;
		}
		assert(static_cast<size_t>(need_nop) >= sizeof(push_arg) + OP_CALL_SIZE + sizeof(cleanup_stack));

		SetMemPatchable(pGetPlayerCall, need_nop);
		fill_nop(pGetPlayerCall, need_nop);

		copy_bytes(push_arg, pGetPlayerCall, sizeof(push_arg));
		pGetPlayerCall += sizeof(push_arg);

		pGetPlayerCall[0] = OP_CALL;
		replace_call_addr(pGetPlayerCall, addr);
		copy_bytes(cleanup_stack, &pGetPlayerCall[OP_CALL_SIZE], sizeof(cleanup_stack));
#endif

		return pGetPlayerCall;
	}
}
