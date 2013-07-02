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
#include "on_recompute_versus_completion.h"
#include "util.h"

#include "CDetour/detourhelpers.h"
#include <stdlib.h>	// for qsort()
#include "routine.h"

/*
typedef struct {
	uint32_t survivors_completion_score[2][4];	// 976 bytes
	uint32_t survivors_death_score[2][4];		// 1008 bytes
	uint8_t survivors_escaped[4];				// 1040 bytes	// in-game buffer
	uint32_t versus_completion_score;			// 1044 bytes	// in-game buffer
	uint32_t versus_survival_multiplier[2];		// 1048 bytes	// user count who survived
	uint8_t unkownData[100];			      	// 1056 bytes	
	uint32_t sacrafice_escaped_mask;			// 1156 bytes	// e.g. 1000 0110 0000 0001 1000 0110 0000 0001 where bit position is a client index
} versus_completion_t;
*/

namespace Detours
{
	int RecomputeVersusCompletion::OnRecompute(bool arg) {		
		int i;
		bool team = AreTeamsFlipped(*g_pDirector);

		i = myRecompute(this);
		if(i > g_totalResult) g_totalResult = i;
		
		uint32_t (*piVersusSurvivorCompletion)[4] = reinterpret_cast<uint32_t(*)[4]>((unsigned char *)(this) + 976);
		
		/**
		* Fix score shows by pressing TAB
		*/
		NotifyNetworkStateChanged();
		
		piVersusSurvivorCompletion[team][0] = 
			piVersusSurvivorCompletion[team][1] = 
				piVersusSurvivorCompletion[team][2] = 
					piVersusSurvivorCompletion[team][3] = g_totalResult / 4;
		
		piVersusSurvivorCompletion[team][0] += g_totalResult % 4;
		
		L4D_DEBUG_LOG("Recompute return: %d.", g_totalResult);
		
		return g_totalResult;
	}
	
	/**
	* Recompute completion for all survivors. Sort results.
	*/
	int RecomputeVersusCompletion::myRecompute(void *pGameRules) {
		// static bool (*IsDead)(CBaseEntity *) = [](CBaseEntity *pPlayer) { return *(bool*)((unsigned char*)pPlayer+252); }; // use it, if you GCC >= 4.5
		static int (*GetVersusCompletionFunc)(void *, CBaseEntity*);
		if(!GetVersusCompletionFunc) {
			if( !g_pGameConf->GetMemSig("CTerrorGameRules_GetVersusCompletion", (void**)&GetVersusCompletionFunc) || !GetVersusCompletionFunc ) {
				g_pSM->LogError(myself, "Can't resolve CTerrorGameRules_GetVersusCompletion signature");
				assert(0);
			}
		}
		
		int client, result = 0;
		CBaseEntity *pPlayer;
		
		uint32_t *pCompl = g_iHighestVersusSurvivorCompletion;
		for(client = 1; client <= 32; ++client) {
			pPlayer = UTIL_GetCBaseEntity(client, true);
			if(pPlayer == NULL) continue;
			
			// if(GET_TEAM(client) == 2) {
			if( *reinterpret_cast<uint8_t*>((unsigned char*)pPlayer + 576) == 2 ) {		// Get player team index
				if(IsDead(pPlayer)) continue;
				*pCompl = g_scores[client] = GetVersusCompletionFunc(pGameRules, pPlayer);
				result += *pCompl++;
			}
		}
		result += r_appendScores(pCompl, TEAM_SIZE - (pCompl - g_iHighestVersusSurvivorCompletion), g_players, sizeof(g_players)/sizeof(g_players[0]));
		
		qsort(g_iHighestVersusSurvivorCompletion, TEAM_SIZE, sizeof(uint32_t), getHighest);
		L4D_DEBUG_LOG("myRecompute return: %d.", result);
		return result;
	}
}