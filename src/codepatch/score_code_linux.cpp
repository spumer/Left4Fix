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

#include "score_code_basic.h"
#include "extension.h"
#include "asm/asm.h"
#include "CDetour/detourhelpers.h"
#include "codepatch/patch_utils.hpp"

#define OP_MOV 0xA1
#define OP_MOV_SIZE 5

// TODO: Create CUT/PASTE masks functions for wrap instructions inside my patch

unsigned char UpdateMarkersReached_orig[]  = { 0x89, 0x04, 0x24, 0xE8, 0x07, 0x9E, 0xC6, 0xFF, 0xF3, 0x0F, 0x2A, 0x45, 0xE4, 0xC1, 0xF8, 0x02 };
unsigned char UpdateMarkersReached_patch[] = { 0x8B, 0x80, 0xFC, 0x0D, 0x00, 0x00, 0x31, 0xD2, 0xBB, TEAM_SIZE, 0x00, 0x00, 0x00, 0xF7, 0xF3, 0xF3, 0x0F, 0x2A, 0x45, 0xE4 };

unsigned char AddSurvivorStats_orig[]  = { 0x89, 0x04, 0x24, 0xE8, 0x89, 0xCB, 0xC4, 0xFF, 0xC1, 0xF8, 0x02, 0xF3, 0x0F, 0x2A, 0xC0, 0x0F, 0x2F, 0x05, 0x90, 0x95, 0xBA, 0x00, 0xF3, 0x0F, 0x11, 0x45, 0xAC };
unsigned char AddSurvivorStats_patch[] = { 0x8B, 0x80, 0xFC, 0x0D, 0x00, 0x00, 0x31, 0xD2, 0xB9, TEAM_SIZE, 0x00, 0x00, 0x00, 0xF7, 0xF1, 0x85, 0xC0, 0x89, 0x45, 0xAC, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};

unsigned char GetVersusCompletion_orig[]  = { 0xB8, 0x00, 0x00, 0x00, 0x00, 0xC1, 0xFB, 0x02, 0x85, 0xDB, 0x0F, 0x48, 0xD8 };
unsigned char GetVersusCompletion_patch[] = { 0x89, 0xD8, 0x31, 0xD2, 0xBB, TEAM_SIZE, 0x00, 0x00, 0x00, 0xF7, 0xF3, 0x89, 0xC3 };


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

	// prepare the trampoline
	m_injectMarker = (unsigned char *)sengine->AllocatePageMemory(sizeof(UpdateMarkersReached_patch) + OP_JMP_SIZE);
	copy_bytes(UpdateMarkersReached_patch, m_injectMarker, sizeof(UpdateMarkersReached_patch));
	inject_jmp(m_injectMarker + sizeof(UpdateMarkersReached_patch), m_pMarkers + OP_JMP_SIZE); // inject jump to position afterward JMP which brought us here

	// copy original code to our buffer
	SetMemPatchable(m_pMarkers, sizeof(UpdateMarkersReached_orig));
	copy_bytes(m_pMarkers, UpdateMarkersReached_orig, sizeof(UpdateMarkersReached_orig));
	// inject jmp to trampoline and nop some bytes after target instruction
	inject_jmp(m_pMarkers, m_injectMarker);
	fill_nop(m_pMarkers + OP_JMP_SIZE, sizeof(UpdateMarkersReached_orig) - OP_JMP_SIZE);

	// // before patch copy original code to our buffer
	SetMemPatchable(m_pL4DStats, sizeof(AddSurvivorStats_orig));
	copy_bytes(m_pL4DStats, AddSurvivorStats_orig, sizeof(AddSurvivorStats_orig));
	copy_bytes(AddSurvivorStats_patch, m_pL4DStats, sizeof(AddSurvivorStats_patch));

	// // before patch copy original code to our buffer
	SetMemPatchable(m_pCompletion, sizeof(GetVersusCompletion_orig));
	copy_bytes(m_pCompletion, GetVersusCompletion_orig, sizeof(GetVersusCompletion_orig));
	copy_bytes(GetVersusCompletion_patch, m_pCompletion, sizeof(GetVersusCompletion_patch));

	m_isPatched = true;
}

void ScoreCode::Unpatch() {
	if(!m_isPatched) return;

	ISourcePawnEngine *sengine = g_pSM->GetScriptingEngine();
	if(m_injectMarker) { copy_bytes(m_pMarkers, UpdateMarkersReached_orig, sizeof(UpdateMarkersReached_orig)); sengine->FreePageMemory(m_injectMarker); }
	copy_bytes(m_pL4DStats, AddSurvivorStats_orig, sizeof(AddSurvivorStats_orig));
	copy_bytes(m_pCompletion, GetVersusCompletion_orig, sizeof(GetVersusCompletion_orig));

	m_isPatched = false;
}
