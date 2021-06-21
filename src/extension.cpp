/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Left4Fix Extension
 * Copyright (C) 2013 Spumer
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
#include "wrappers.h"
#include <igameevents.h>

#include "codepatch/patchmanager.h"
#include "codepatch/autopatch.h"
#include "codepatch/score_code.h"
#include "codepatch/warp_max_players.h"
#include "codepatch/tiebreak.h"

#include "detours/on_get_completion_by_character.h"
#include "detours/on_recompute_versus_completion.h"
#include "detours/on_warp_ghost_call_get_player.h"
#include "detours/on_revived_by_defib.h"
#include "event_player_death.hpp"
#include "event_round_start.hpp"

#include <CDetour/detours.h>


DETOUR_DECL_MEMBER0(Handler_CFrameSnapshotManager_LevelChanged, void)
{
	CFrameSnapshotManager* _this = reinterpret_cast<CFrameSnapshotManager*>(this);

	// bug##: Underlying method CClassMemoryPool::Clear creates CUtlRBTree with insufficient iterator size (unsigned short)
	// memory object of which fails to iterate over more than 65535 of packed entities
	_this->m_PackedEntitiesPool().Clear();

	// CFrameSnapshotManager::m_PackedEntitiesPool shouldn't have elements to free from now on
	DETOUR_MEMBER_CALL(Handler_CFrameSnapshotManager_LevelChanged)();
}

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

Left4Fix g_Left4Fix;		/**< Global singleton for extension's main interface */
PatchManager g_PatchManager;
CGlobalVars *gpGlobals;
PlayerDeath g_OnPlayerDeath;
RoundStart g_OnRoundStart;
void **g_pDirector = NULL;
extern sp_nativeinfo_t g_Left4FixNatives[];

namespace Detours {
	int g_totalResult = 0;
	int g_versusSurvivorCompletionOffset;
	death_info_t g_dead_players[32];
	uint32_t g_scores[32+1] = {0};
	bool (__thiscall *AreTeamsFlipped)(const void*);
	void (__thiscall *NotifyNetworkStateChanged)(void);
	uint32_t g_iHighestVersusSurvivorCompletion[TEAM_SIZE] = {0};
};


/* Interfaces */
IGameConfig *g_pGameConf = NULL;
ISmmAPI *g_pSmmAPI = NULL;
ICvar *g_pCVar = NULL;
IGameEventManager2 *gameevents = NULL;

SMEXT_LINK(&g_Left4Fix);


void** get_director_pointer(char *error, size_t maxlength) {
	void *addr;

#ifdef WIN32
	int offset = 0;
	if (!g_pGameConf->GetMemSig("DirectorMusicBanks_OnRoundStart", (void **)&addr) || !addr)
	{
		UTIL_Format(error, maxlength, "Could not read DirectorMusicBanks_OnRoundStart signature");
		return nullptr;
	}
	if (!g_pGameConf->GetOffset("TheDirector", &offset) || !offset)
	{
		UTIL_Format(error, maxlength, "Could not read pDirector signature");
		return nullptr;
	}
	return *reinterpret_cast<void ***>((unsigned char*)addr + offset);
#else
	if(!g_pGameConf->GetMemSig("g_pDirector", (void **)&addr) || !addr)
	{
		UTIL_Format(error, maxlength, "Could not read pDirector signature");
		return nullptr;
	}
	return reinterpret_cast<void **>(addr);
#endif
}


bool Left4Fix::SDK_OnLoad(char *error, size_t maxlength, bool late) {
	char conf_error[255] = "";

	if (!gameconfs->LoadGameConfigFile(GAMECONFIG_FILE, &g_pGameConf, conf_error, sizeof(conf_error))) {
		if (conf_error[0]) {
			UTIL_Format(error, maxlength, "Could not read " GAMECONFIG_FILE ".txt: %s", conf_error);
		}
		return false;
	}

	g_pDirector = get_director_pointer(error, maxlength);
	if(!g_pDirector)
	{
		return false;
	}

	if(!g_pGameConf->GetMemSig("CDirector_AreTeamsFlipped", (void **)&Detours::AreTeamsFlipped) || !Detours::AreTeamsFlipped)
	{
	    UTIL_Format(error, maxlength, "Could not read CDirector_AreTeamsFlipped signature");
		return false;
	}

	if(!g_pGameConf->GetMemSig("CGameRulesProxy_NotifyNetworkStateChanged", (void **)&Detours::NotifyNetworkStateChanged) || !Detours::NotifyNetworkStateChanged)
	{
	    UTIL_Format(error, maxlength, "Could not read CGameRulesProxy_NotifyNetworkStateChanged signature");
		return false;
	}

	if (!g_pGameConf->GetOffset("VersusCompletionScore", &g_versusSurvivorCompletionOffset) || !g_versusSurvivorCompletionOffset)
	{
		UTIL_Format(error, maxlength, "Could not read VersusCompletionScore offset");
		return false;
	}

	memset(g_dead_players, 0, sizeof(g_dead_players));

	gameevents->AddListener(&g_OnPlayerDeath, "player_death", true);
	gameevents->AddListener(&g_OnRoundStart,  "round_start",  true);

	Detour::Init(g_pSM->GetScriptingEngine(), g_pGameConf);

    if (!CreateDetours(error, maxlength))
    {
        return false;
    }

    CFrameSnapshotManager::detour_LevelChanged->EnableDetour();

	return true;
}

void Left4Fix::SDK_OnAllLoaded() {
	g_PatchManager.Register(new AutoPatch<ScoreCode>());
	g_PatchManager.Register(new AutoPatch<Detours::RecomputeVersusCompletion>());
	g_PatchManager.Register(new AutoPatch<Detours::OnGetCompletionByCharacter>());
	g_PatchManager.Register(new AutoPatch<Detours::RevivedByDefib>());
	g_PatchManager.Register(new AutoPatch<Detours::OnWarpGhostCallGetPlayer>());
	g_PatchManager.Register(new AutoPatch<WarpMaxPlayers>());
	g_PatchManager.Register(new AutoPatch<Tiebreak>());

	sharesys->AddNatives(myself, g_Left4FixNatives);
	sharesys->RegisterLibrary(myself, "Left4Fix");
}

void Left4Fix::SDK_OnUnload() {

	if (CFrameSnapshotManager::detour_LevelChanged != NULL) {
		CFrameSnapshotManager::detour_LevelChanged->Destroy();
		CFrameSnapshotManager::detour_LevelChanged = NULL;
	}

	g_PatchManager.UnregisterAll();
	gameconfs->CloseGameConfigFile(g_pGameConf);
}

bool Left4Fix::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late) {
	g_pSmmAPI = ismm;

	GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, gameevents, IGameEventManager2, INTERFACEVERSION_GAMEEVENTSMANAGER2);
	gpGlobals = g_pSmmAPI->GetCGlobals();
	return true;
}

bool Left4Fix::SDK_OnMetamodUnload(char *error, size_t maxlength) {
	return true;
}


int CFrameSnapshotManager::offset_m_PackedEntitiesPool = 0;
void* CFrameSnapshotManager::pfn_LevelChanged = NULL;
CDetour* CFrameSnapshotManager::detour_LevelChanged = NULL;


bool Left4Fix::CreateDetours(char* error, size_t maxlength)
{
    CDetourManager::Init(g_pSM->GetScriptingEngine(), g_pGameConf);
    IGameConfig* gc = g_pGameConf;

    static const struct {
		const char* key;
		int& offset;
	} s_offsets[] = {
		{ "CFrameSnapshotManager::m_PackedEntitiesPool", CFrameSnapshotManager::offset_m_PackedEntitiesPool },
	};

	for (auto&& el : s_offsets) {
		if (!gc->GetOffset(el.key, &el.offset)) {
			UTIL_Format(error, maxlength, "Unable to get offset for \"%s\" from game config (file: \"" GAMEDATA_FILE ".txt\")", el.key);
			return false;
		}
	}

	static const struct {
		const char* key;
		void*& address;
	} s_sigs[] = {
		{ "CFrameSnapshotManager::LevelChanged", CFrameSnapshotManager::pfn_LevelChanged },
	};

	for (auto&& el : s_sigs) {
		if (!gc->GetMemSig(el.key, &el.address)) {
			UTIL_Format(error, maxlength, "Unable to find signature for \"%s\" from game config (file: \"" GAMEDATA_FILE ".txt\")", el.key);

			return false;
		}

		if (el.address == NULL) {
			UTIL_Format(error, maxlength, "Sigscan for \"%s\" failed (game config file: \"" GAMEDATA_FILE ".txt\")", el.key);

			return false;
		}
	}

	CFrameSnapshotManager::detour_LevelChanged = DETOUR_CREATE_MEMBER(Handler_CFrameSnapshotManager_LevelChanged, CFrameSnapshotManager::pfn_LevelChanged);
	if (CFrameSnapshotManager::detour_LevelChanged == NULL) {
		UTIL_Format(error, maxlength, "Unable to create a detour for \"CFrameSnapshotManager::LevelChanged\"");
		return false;
	}

    return true;
}

//


/*
                                                 dd
                                           dd
                                     dd
               ,;,_ _,;;,      dd
            .^//// v \\\\\^.
           /'/// =    = \\\'\
          / ;//d   __   b\\'.'.
         ;  '.  `._  _.'    ;  '.
        .    ;    _)(_     .    ;
        .    '. .'    '.   .    '.
        '.    '(.      .)   '.   '.
         ;     ;\\.)(.//     ;    ;
        .     .  \\  //     .    .        lb
       ;     ;   )\\//(     ;     ;
       '.    '. (  ww' )    '.    '.
         `v`. ;  './ .'       `v`. ;
             v    / / \           v
*/
