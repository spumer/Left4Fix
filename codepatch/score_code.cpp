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

#include "score_code.h"
#include "extension.h"
#include "asm/asm.h"
#include "CDetour/detourhelpers.h"

// TODO: Replace sig search to offsets, copy original by copy_bytes and restore when unpatch.

unsigned char UpdateMarkersReached_orig[]  = { 0xE8, 0x35, 0x69, 0xBA, 0xFF, 0xC1, 0xF8, 0x02 };
unsigned char UpdateMarkersReached_patch[] = { 0x8B, 0x80, 0x90, 0x04, 0x00, 0x00, 0x31, 0xD2, 0xB9, TEAM_SIZE, 0x00, 0x00, 0x00, 0xF7, 0xF1 };

unsigned char AddSurvivorStats_orig[]  = { 0xE8, 0xF1, 0x9C, 0xB7, 0xFF, 0xC1, 0xF8, 0x02 };
unsigned char AddSurvivorStats_patch[] = { 0x8B, 0x80, 0x90, 0x04, 0x00, 0x00, 0x31, 0xD2, 0xB9, TEAM_SIZE, 0x00, 0x00, 0x00, 0xF7, 0xF1 };

unsigned char GetVersusCompletion_orig[]  = { 0x8B, 0xB8, 0x90, 0x04, 0x00, 0x00, 0xC1, 0xFF, 0x02 };
unsigned char GetVersusCompletion_patch[] = { 0x8B, 0x80, 0x90, 0x04, 0x00, 0x00, 0x31, 0xD2, 0xBF, TEAM_SIZE, 0x00, 0x00, 0x00, 0xF7, 0xF7, 0x89, 0xC7 };

#ifdef _DEBUG
void memDump(unsigned char *pAddr, size_t len) {
	g_pSmmAPI->ConPrintf("Start dump at: %p\n", pAddr);
	size_t llen = len;
	while(len--) {
		g_pSmmAPI->ConPrintf("%02x", *pAddr++ & 0xFF);
	}
	g_pSmmAPI->ConPrintf("\nDump end. Next byte: %p. Len: %d\n", pAddr, llen);
}
#endif

void ScoreCode::Patch() {
	if(m_isPatched) return;
	
	g_pGameConf->GetMemSig("DIV_CODE_UpdateMarkersReached", (void **)&m_pMarkers);
	g_pGameConf->GetMemSig("DIV_CODE_AddSurvivorStats", (void **)&m_pL4DStats);
	g_pGameConf->GetMemSig("DIV_CODE_GetVersusCompletion", (void**)&m_pCompletion);
	if( !m_pMarkers || !m_pL4DStats || !m_pCompletion ) {
		g_pSM->LogError(myself, "Can't get one of the \"scorecode\" signatures: %x/%x/%x", m_pMarkers, m_pL4DStats, m_pCompletion);
		return;
	}
	
	ISourcePawnEngine *sengine = g_pSM->GetScriptingEngine();
	
	m_injectMarker = (unsigned char *)sengine->AllocatePageMemory(sizeof(UpdateMarkersReached_patch) + OP_JMP_SIZE);
	copy_bytes(UpdateMarkersReached_patch, m_injectMarker, sizeof(UpdateMarkersReached_patch));
	inject_jmp(m_injectMarker + sizeof(UpdateMarkersReached_patch), m_pMarkers + sizeof(UpdateMarkersReached_orig));
	SetMemPatchable(m_pMarkers, sizeof(UpdateMarkersReached_orig));
	inject_jmp(m_pMarkers, m_injectMarker);
	fill_nop(m_pMarkers + OP_JMP_SIZE, sizeof(UpdateMarkersReached_orig) - OP_JMP_SIZE);
	
	m_injectStats = (unsigned char *)sengine->AllocatePageMemory(sizeof(AddSurvivorStats_patch) + OP_JMP_SIZE);
	copy_bytes(AddSurvivorStats_patch, m_injectStats, sizeof(AddSurvivorStats_patch));
	inject_jmp(m_injectStats + sizeof(AddSurvivorStats_patch), m_pL4DStats + sizeof(AddSurvivorStats_orig));	
	SetMemPatchable(m_pL4DStats, sizeof(AddSurvivorStats_orig));
	inject_jmp(m_pL4DStats, m_injectStats);
	fill_nop(m_pL4DStats + OP_JMP_SIZE, sizeof(AddSurvivorStats_orig) - OP_JMP_SIZE);
	
	m_injectCompl = (unsigned char *)sengine->AllocatePageMemory(sizeof(GetVersusCompletion_patch) + OP_JMP_SIZE);
	copy_bytes(GetVersusCompletion_patch, m_injectCompl, sizeof(GetVersusCompletion_patch));
	inject_jmp(m_injectCompl + sizeof(GetVersusCompletion_patch), m_pCompletion + sizeof(GetVersusCompletion_orig));	
	SetMemPatchable(m_pCompletion, sizeof(GetVersusCompletion_orig));
	inject_jmp(m_pCompletion, m_injectCompl);
	fill_nop(m_pCompletion + OP_JMP_SIZE, sizeof(GetVersusCompletion_orig) - OP_JMP_SIZE);
	
	m_isPatched = true;
}

void ScoreCode::Unpatch() {
	if(!m_isPatched) return;
	
	ISourcePawnEngine *sengine = g_pSM->GetScriptingEngine();
	if(m_injectMarker) { copy_bytes(m_pMarkers, UpdateMarkersReached_orig, sizeof(UpdateMarkersReached_orig)); sengine->FreePageMemory(m_injectMarker); }
	if(m_injectStats) { copy_bytes(m_pL4DStats, AddSurvivorStats_orig, sizeof(AddSurvivorStats_orig)); sengine->FreePageMemory(m_injectStats); }
	if(m_injectCompl) { copy_bytes(m_pCompletion, GetVersusCompletion_orig, sizeof(GetVersusCompletion_orig)); sengine->FreePageMemory(m_injectCompl); }
	
	m_isPatched = false;
}