/**
 * natives.cpp
 * This file is part of HLXRanker
 *
 * Copyright (C) 2013 - spumer
 *
 * HLXRanker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * HLXRanker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HLXRanker. If not, see <http://www.gnu.org/licenses/>.
 */

#include "extension.h"


cell_t GetSurvivorScore(IPluginContext *pContext, const cell_t *params) {
	CBaseEntity *pPlayer = gamehelpers->ReferenceToEntity(params[1]);
	if (!pPlayer)
		return pContext->ThrowNativeError("Invalid client index %d", params[1]);

	cell_t client = gamehelpers->ReferenceToIndex(params[1]);

	IGamePlayer *pGamePlayer = playerhelpers->GetGamePlayer(client);
	if (!pGamePlayer || !pGamePlayer->IsInGame())
		return pContext->ThrowNativeError("Client index %d not in game", params[1]);

	IPlayerInfo* pInfo = pGamePlayer->GetPlayerInfo();
	if(!pInfo || pInfo->IsObserver() || pInfo->GetTeamIndex() != 2)
		return -1;

	return Detours::g_scores[client];
}

cell_t GetSupportedTeamSize(IPluginContext *pContext, const cell_t *params) {
	return TEAM_SIZE;
}


sp_nativeinfo_t g_Left4FixNatives[] = {
	{"L4FIX_GetSurvivorScore",	GetSurvivorScore},
	{"L4FIX_GetSupportedTeamSize", GetSupportedTeamSize},
	{NULL,					NULL}
};
