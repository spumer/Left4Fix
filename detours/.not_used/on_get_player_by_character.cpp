/**
 * vim: set ts=4 :
 * =============================================================================
 * Left 4 Fix SourceMod Extension
 * Copyright (C) 2013 Spumer.
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
#include "on_get_player_by_character.h"
 
namespace Detours
{
	void* OnGetPlayerByCharacter::OnGetPlayer(uint8_t survivorType) {
		static int client = 0;
		if(g_RecomputeIsActive) {
			void *newPlayer = NULL;
			IGamePlayer *pPlayer;
			
			for(++client; client <= 32; ++client) {
				newPlayer = UTIL_GetCBaseEntity(client, true);
				if(newPlayer == NULL) continue;
				
				pPlayer = playerhelpers->GetGamePlayer(client);
				if(pPlayer->GetPlayerInfo()->GetTeamIndex() == 2) {
					L4D_DEBUG_LOG("Survivor is: %x", newPlayer);
					return newPlayer;
				}
			}
		} else {
			client = 0;	// now we leave recompute function and should reset var*/
		}
		return (this->*(GetTrampoline()))(survivorType);
	}
}