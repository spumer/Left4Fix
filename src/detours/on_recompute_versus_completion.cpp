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

#include <shareddefs.h>	// hl2sdk definitions

/*
typedef struct {
	uint8_t unknownData[908];					// 0 bytes
	uint32_t versus_score_bonus[2];				// 908 bytes	// temp buffer to store chapter score for second team
	uint32_t versus_campaign_score[2];			// 916 bytes
	uint32_t chapter_score[2];					// 924 bytes	// full filled only after EndVersusRoundEnd
	uint8_t unknownData1[72];					// 932 bytes
	uint32_t survivors_completion_score[2][4];	// 996 bytes
	uint32_t survivors_death_score[2][4];		// 1028 bytes
	uint8_t survivors_escaped[4];				// 1060 bytes	// in-game buffer
	uint32_t versus_completion_score;			// 1064 bytes	// in-game buffer
	uint32_t versus_survival_multiplier[2];		// 1068 bytes	// user count who survived
	uint8_t unknownData2[92];			      	// 1076 bytes
	uint8_t areTeamsFlipped						// 1168 bytes
	uint8_t roundFirstHalf						// 1169 bytes
	uint8_t isTransitionToNextMap;		      	// 1170 bytes
	uint8_t unknownData3[5];				    // 1171 bytes
	uint32_t sacrafice_escaped_mask;			// 1176 bytes	// e.g. 1000 0110 0000 0001 1000 0110 0000 0001 where bit position is a client index
} CTerrorGameRules_t;
*/

namespace Detours
{
	int cmp_uint_desc(const void* p1, const void* p2) {
		return static_cast<int>(*(uint32_t*)p2 - *(uint32_t*)p1);
	}

	int __thiscall RecomputeVersusCompletion::OnRecompute(bool arg) {
		int newTotalResult = myRecompute(this);
		if(newTotalResult <= g_totalResult) {
			return g_totalResult;
		}

		g_totalResult = newTotalResult;

		uint32_t (*piVersusSurvivorCompletion)[4] = reinterpret_cast<uint32_t(*)[4]>((unsigned char *)(this) + g_versusSurvivorCompletionOffset);

		/**
		* Fix score shows by pressing TAB
		*/

		bool team = AreTeamsFlipped(*g_pDirector);

		piVersusSurvivorCompletion[team][0] =
			piVersusSurvivorCompletion[team][1] =
				piVersusSurvivorCompletion[team][2] =
					piVersusSurvivorCompletion[team][3] = g_totalResult / 4;

		piVersusSurvivorCompletion[team][0] += g_totalResult % 4;

		NotifyNetworkStateChanged();

		L4D_DEBUG_LOG("Recompute return: %d.", g_totalResult);

		return g_totalResult;
	}

	/**
	* Recompute completion for all survivors. Sort results.
	*/
	int RecomputeVersusCompletion::myRecompute(void *pGameRules) {
		static int (__thiscall *GetVersusCompletionFunc)(void *, CBaseEntity*);
		if(!GetVersusCompletionFunc) {
			if( !g_pGameConf->GetMemSig("CTerrorGameRules_GetVersusCompletion", (void**)&GetVersusCompletionFunc) || !GetVersusCompletionFunc ) {
				g_pSM->LogError(myself, "Can't resolve CTerrorGameRules_GetVersusCompletion signature");
				assert(0);
			}
		}

		int result = 0;
		CBaseEntity *pPlayer;
		IPlayerInfo *pInfo;
		IGamePlayer *pGamePlayer;

		uint32_t *pNextScore = g_iHighestVersusSurvivorCompletion;
		for(int client = 1, survivors = 0; client <= 32; ++client) {
			pPlayer = gamehelpers->ReferenceToEntity(client);
			if(!pPlayer) continue;

			pGamePlayer = playerhelpers->GetGamePlayer(client);
			if(!pGamePlayer || !pGamePlayer->IsConnected()) continue;

			pInfo = pGamePlayer->GetPlayerInfo();
			if(!pInfo) continue;

			if(pInfo->GetTeamIndex() == 2) {
				if(pInfo->IsObserver()) continue;

				// Save actual score for player
				g_scores[client] = GetVersusCompletionFunc(pGameRules, pPlayer);
				L4D_DEBUG_LOG("Player: index=%d, name=%s, score=%d", client, pInfo->GetName(), g_scores[client]);

				// No reason add zero to result
				// This actually fix the situation when survivor changed the team
				// Team changing leads to create additional bot and then add him to team
				// Only after this real player will be placed to requested team
				// When changing team in progress the both players (bot and human) has a 0 points
				if(g_scores[client] != 0) {
					if(++survivors > TEAM_SIZE) {
						// Break loop when g_iHighestVersusSurvivorCompletion buffer can't take next value
						g_pSM->LogError(myself, "Attention! TEAM_SIZE limit is exceeded. Extension don't support more than %d players", TEAM_SIZE);
						break;
					}

					*pNextScore = g_scores[client];
					result += *pNextScore++;
				}
			}
		}

		result += r_appendScores(pNextScore, TEAM_SIZE - (pNextScore - g_iHighestVersusSurvivorCompletion), g_dead_players, arraysize(g_dead_players));

		qsort(g_iHighestVersusSurvivorCompletion, TEAM_SIZE, sizeof(uint32_t), cmp_uint_desc);
		return result;
	}
}
