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

#include "score_code_8.h"
#include "extension.h"
#include "CDetour/detourhelpers.h"

void ScoreCode::Patch() {
	int offset;
	g_pGameConf->GetMemSig("CDirector_UpdateMarkersReached", (void **)&m_pDivisor);
	g_pGameConf->GetMemSig("CL4DGameStats_AddSurvivorStats", (void **)&m_pDivisor2);
	g_pGameConf->GetMemSig("CTerrorGameRules_GetVersusCompletion", (void**)&m_pDivisor3);
	
	g_pGameConf->GetOffset("MarkerDivisor", &offset);
	if(m_pDivisor && offset) {
		m_pDivisor += offset;
		SetMemPatchable(m_pDivisor, 1);
		(*m_pDivisor) = 3;
	}
	
	g_pGameConf->GetOffset("StatsDivisor", &offset);
	if(m_pDivisor2 && offset) {
		m_pDivisor2 += offset;
		SetMemPatchable(m_pDivisor2, 1);
		(*m_pDivisor2) = 3;
	}
	
	g_pGameConf->GetOffset("PerPlayerCompletionDivisor", &offset);
	if(m_pDivisor3 && offset) {
		m_pDivisor3 += offset;
		SetMemPatchable(m_pDivisor3, 1);
		(*m_pDivisor3) = 3;
	}
	// g_pSM->LogMessage(myself, "Divisors: (%d) (%d) (%d)", *m_pDivisor & 0xFF, *m_pDivisor2 & 0xFF, *m_pDivisor3 & 0xFF);
}

void ScoreCode::Unpatch() {
	if(m_pDivisor) *m_pDivisor = 0x02;
	if(m_pDivisor2)*m_pDivisor2= 0x02;
	if(m_pDivisor3)*m_pDivisor3= 0x02;
}