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

#include "asm/asm.h"
#include "tiebreak.h"
#include "extension.h"
#include "CDetour/detourhelpers.h"


// for more details see the tiebreak_patch_info.txt
#ifdef WIN32
#define BYTES_PART_ONE 5
#define BYTES_PART_TWO 6
const uint8_t g_TieBreak_patch[] = {
    0x89, 0xCB, 0x6A, 0x01, 0x53,
    0xE8, 0xFF, 0xFF, 0xFF, 0xFF,
    0x89, 0x45, 0xFC, 0x6A, 0x02, 0x53,
    0xE8, 0xFF, 0xFF, 0xFF, 0xFF
};
#else
#define BYTES_PART_ONE 13
#define BYTES_PART_TWO 14
const uint8_t g_TieBreak_patch[] =  {
	0x89, 0xC3, 0xC7, 0x44, 0x24, 0x04, 0x01, 0x00, 0x00, 0x00, 0x89, 0x1C, 0x24,
	0xE8, 0xFF, 0xFF, 0xFF, 0xFF,
	0x89, 0x45, 0xC4, 0xC7, 0x44, 0x24, 0x04, 0x02, 0x00, 0x00, 0x00, 0x89, 0x1C, 0x24,
	0xE8, 0xFF, 0xFF, 0xFF, 0xFF
};
#endif

typedef struct TiebreakPatch_t {
	uint8_t bytes[BYTES_PART_ONE];
	uint8_t call_chapter_score_a[OP_CALL_SIZE];
	uint8_t bytes2[BYTES_PART_TWO];
	uint8_t call_chapter_score_b[OP_CALL_SIZE];
} *pTiebreakPatch_t;

#ifdef WIN32
int __stdcall GetScoreDetour(void* pGameRules, int team) {
#else
int GetScoreDetour(void* pGameRules, int team) {
#endif
	L4D_DEBUG_LOG("Called GetScoreDetour: this=%x, team=%d", pGameRules, team);
	static int (__thiscall *CTerrorGameRules_GetChapterScore)(void* pGameRules, int team);
	static int (__thiscall *CTerrorGameRules_GetTeamScore)(void* pGameRules, int team, bool campaign);

	if( !CTerrorGameRules_GetChapterScore ) {
		if( !g_pGameConf->GetMemSig("CTerrorGameRules_GetChapterScore", (void**)&CTerrorGameRules_GetChapterScore) || !CTerrorGameRules_GetChapterScore ) {
			g_pSM->LogError(myself, "Can't resolve CTerrorGameRules_GetChapterScore signature");
			assert(0);
		}
	}
	if( !CTerrorGameRules_GetTeamScore ) {
		if( !g_pGameConf->GetMemSig("CTerrorGameRules_GetTeamScore", (void**)&CTerrorGameRules_GetTeamScore) || !CTerrorGameRules_GetTeamScore ) {
			g_pSM->LogError(myself, "Can't resolve CTerrorGameRules_GetTeamScore signature");
			assert(0);
		}
	}

	int result = CTerrorGameRules_GetChapterScore(pGameRules, team);
	if(!result) {
		// This detour called when score not moved yet to chapter section for second team
		// And we retrieve it from source
		result = CTerrorGameRules_GetTeamScore(pGameRules, team, false);
	}

	L4D_DEBUG_LOG("GetScoreDetour: result=%d", result);
	return result;
}


void Tiebreak::Patch() {
	if(m_isPatched) return;

	int need_nop;
	pTiebreakPatch_t tiebreak_patch;

	if( !g_pGameConf->GetMemSig("CHECK_CODE_Tiebreak", (void **)&m_regionAddr) || !m_regionAddr ) {
		g_pSM->LogError(myself, "Tiebreak -- Could not find 'CHECK_CODE_Tiebreak address'");
		return;
	}

	if( !g_pGameConf->GetOffset("Tiebreak_CheckCodeLen", &need_nop) || !need_nop ) {
		g_pSM->LogError(myself, "Tiebreak -- Could not find 'Tiebreak_CheckCodeLen'");
		return;
	}

	assert(static_cast<size_t>(need_nop) >= sizeof(TiebreakPatch_t));

	SetMemPatchable(m_regionAddr, need_nop);

	m_regionData = new uint8_t[need_nop];
	m_regionLen = need_nop;
	copy_bytes(/*src*/(unsigned char *)m_regionAddr, /*dst*/m_regionData, m_regionLen);

	fill_nop(m_regionAddr, need_nop);

	tiebreak_patch = reinterpret_cast<pTiebreakPatch_t>(m_regionAddr);
	copy_bytes(/*src*/(unsigned char *)g_TieBreak_patch, /*dst*/(unsigned char *)tiebreak_patch, /*len*/sizeof(TiebreakPatch_t));
	replace_call_addr(tiebreak_patch->call_chapter_score_a, (void *)GetScoreDetour);
	replace_call_addr(tiebreak_patch->call_chapter_score_b, (void *)GetScoreDetour);

	m_isPatched = true;
	L4D_DEBUG_LOG("Tiebreak code patched successfully");
}

void Tiebreak::Unpatch() {
	if(!m_isPatched) return;

	if(m_regionData) {
		copy_bytes(m_regionData, m_regionAddr, m_regionLen);
		delete[] m_regionData;
	}

	m_isPatched = false;
}
