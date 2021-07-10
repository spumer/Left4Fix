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

#include <asm/asm.h>
#include "score_code_basic.h"
#include "extension.h"
#include "CDetour/detourhelpers.h"

#define OP_MOV 0xA1
#define OP_MOV_SIZE 5

unsigned char UpdateMarkersReached_patch[] = { 0x8B, 0x80, 0xFC, 0x0D, 0x00, 0x00, 0x31, 0xD2, 0xB9, TEAM_SIZE, 0x00, 0x00, 0x00, 0xF7, 0xF1, 0xF3, 0x0F, 0x2A, 0xC6, 0x8B, 0x53, 0x4C };
unsigned char UpdateMarkersReached_orig[sizeof(UpdateMarkersReached_patch)];

unsigned char AddSurvivorStats_patch[] = { 0x8B, 0x80, 0xFC, 0x0D, 0x00, 0x00, 0x31, 0xD2, 0xB9, TEAM_SIZE, 0x00, 0x00, 0x00, 0xF7, 0xF1, 0x85, 0xC0, 0x89, 0x45, 0xB0, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
unsigned char AddSurvivorStats_orig[sizeof(AddSurvivorStats_patch)];

unsigned char GetVersusCompletion_patch[] = { 0x89, 0xF0, 0x31, 0xD2, 0xBE, TEAM_SIZE, 0x00, 0x00, 0x00, 0xF7, 0xF6, 0x89, 0xC6 };
unsigned char GetVersusCompletion_orig[sizeof(GetVersusCompletion_patch)];


void ScoreCode::Patch() {
	if(m_isPatched) return;

	g_pGameConf->GetMemSig("UpdateMarkersReached_search", (void **)&m_pMarkers);
	g_pGameConf->GetMemSig("DIV_CODE_AddSurvivorStats", (void **)&m_pL4DStats);
	g_pGameConf->GetMemSig("DIV_CODE_GetVersusCompletion", (void**)&m_pCompletion);
	if( !m_pMarkers || !m_pL4DStats || !m_pCompletion ) {
		g_pSM->LogError(myself, "Can't get one of the \"scorecode\" signatures: %x/%x/%x", m_pMarkers, m_pL4DStats, m_pCompletion);
		return;
	}

	ISourcePawnEngine *sengine = g_pSM->GetScriptingEngine();

	// TODO: move UpdateMarkersReached_patch to gamedata 'UpdateMarkersReached_replace' and replace TEAM_SIZE by 0x2B placeholder
	// TODO: <key>_replace support
	// TODO: verify <key>_patt

	// prepare the trampoline
	m_injectMarker = (unsigned char *)sengine->AllocatePageMemory(sizeof(UpdateMarkersReached_patch) + OP_JMP_SIZE);
	copy_bytes(UpdateMarkersReached_patch, m_injectMarker, sizeof(UpdateMarkersReached_patch));
	inject_jmp(m_injectMarker + sizeof(UpdateMarkersReached_patch), m_pMarkers + OP_JMP_SIZE); // inject jump to position afterward JMP which brought us here

	// copy original code to our buffer
	const char* patt_UpdateMarkersReached = g_pGameConf->GetKeyValue("UpdateMarkersReached_patt");
	if( !patt_UpdateMarkersReached ) {
		g_pSM->LogError(myself, "Can't get UpdateMarkersReached_patt");
		return;
	}
	m_restoreMarker.bytes = UTIL_DecodeHexString(m_restoreMarker.patch, sizeof(m_restoreMarker.patch), patt_UpdateMarkersReached);
	assert(m_restoreMarker.bytes <= sizeof(m_restoreMarker.patch));

	SetMemPatchable(m_pMarkers, m_restoreMarker.bytes);
	copy_bytes(m_pMarkers, m_restoreMarker.patch, m_restoreMarker.bytes);

	// inject jmp to trampoline
	inject_jmp(m_pMarkers, m_injectMarker);

	const char* nop_UpdateMarkersReached = g_pGameConf->GetKeyValue("UpdateMarkersReached_nop");
	if (nop_UpdateMarkersReached && nop_UpdateMarkersReached[0] == '1')
	{
		fill_nop(m_pMarkers + OP_JMP_SIZE, m_restoreMarker.bytes - OP_JMP_SIZE);
	}

	// before patch copy original code to our buffer
	SetMemPatchable(m_pL4DStats, sizeof(AddSurvivorStats_patch));
	copy_bytes(m_pL4DStats, AddSurvivorStats_orig, sizeof(AddSurvivorStats_orig));
	copy_bytes(AddSurvivorStats_patch, m_pL4DStats, sizeof(AddSurvivorStats_patch));

	// before patch copy original code to our buffer
	SetMemPatchable(m_pCompletion, sizeof(GetVersusCompletion_patch));
	copy_bytes(m_pCompletion, GetVersusCompletion_orig, sizeof(GetVersusCompletion_orig));
	copy_bytes(GetVersusCompletion_patch, m_pCompletion, sizeof(GetVersusCompletion_patch));

	m_isPatched = true;
}

void ScoreCode::Unpatch() {
	if(!m_isPatched) return;

	ISourcePawnEngine *sengine = g_pSM->GetScriptingEngine();
	if(m_injectMarker) {
		copy_bytes(m_restoreMarker.patch, m_pMarkers, m_restoreMarker.bytes);
		sengine->FreePageMemory(m_injectMarker);
		m_injectMarker = nullptr;
	}
	copy_bytes(AddSurvivorStats_orig, m_pL4DStats, sizeof(AddSurvivorStats_orig));
	copy_bytes(GetVersusCompletion_orig, m_pCompletion, sizeof(GetVersusCompletion_orig));

	m_isPatched = false;
}
