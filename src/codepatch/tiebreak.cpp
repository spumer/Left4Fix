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

#include "tiebreak.h"
#include "extension.h"
#include "CDetour/detourhelpers.h"


void Tiebreak::Patch() {
	if(m_isPatched) return;

	uint8_t *base;
	int teamA_offset;
	int teamB_offset;

	if( !g_pGameConf->GetMemSig("CDirectorVersusMode_EndVersusModeRound", (void **)&base) || !base ) {
		g_pSM->LogError(myself, "Tiebreak -- Could not find 'base address'");
		return;		
	}

	if( !g_pGameConf->GetOffset("Tiebreak_TEAM_SIZE_A", &teamA_offset) || !teamA_offset ) {
		g_pSM->LogError(myself, "Tiebreak -- Could not find 'TEAM_SIZE A offset'");
		return;
	}

	if( !g_pGameConf->GetOffset("Tiebreak_TEAM_SIZE_B", &teamB_offset) || !teamB_offset ) {
		g_pSM->LogError(myself, "Tiebreak -- Could not find 'TEAM_SIZE B offset'");
		return;
	}

	m_pTeamSizeA = base + teamA_offset;
	m_pTeamSizeB = base + teamB_offset;

	SetMemPatchable(m_pTeamSizeB, 1);
	SetMemPatchable(m_pTeamSizeA, 1);

	if(*m_pTeamSizeA != 4) {
		g_pSM->LogError(myself, "Tiebreak TEAM_SIZE A is not default: %d", *m_pTeamSizeA);
		return;
	}

	if(*m_pTeamSizeB != 4) {
		g_pSM->LogError(myself, "Tiebreak TEAM_SIZE B is not default: %d", *m_pTeamSizeB);
		return;
	}

	*m_pTeamSizeA = *m_pTeamSizeB = TEAM_SIZE;

	m_isPatched = true;
}

void Tiebreak::Unpatch() {
	if(!m_isPatched) return;

	if(m_pTeamSizeA) *m_pTeamSizeA = 4;
	if(m_pTeamSizeB) *m_pTeamSizeB = 4;

	m_isPatched = false;
}