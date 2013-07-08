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
#include <igameevents.h>
// #include <unistd.h>

#include "codepatch/patchmanager.h"
#include "codepatch/autopatch.h"
#include "codepatch/score_code.h"
 
#include "detours/end_versus_mode_round.h"
#include "detours/on_get_completion_by_character.h"
#include "detours/on_recompute_versus_completion.h"
#include "detours/on_revived_by_defib.h"

#include "event_player_death.hpp"
#include "event_round_start.hpp"

#define GAMECONFIG_FILE "left4fix.sig"

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
// void **g_pGameRules = NULL;

namespace Detours {
	int g_totalResult = 0;
	bool g_bRoundEnd_Pre = false;
	death_info_t g_players[32];		// init in SDK_OnLoad()
	uint32_t g_scores[33] = {0};
	bool (*AreTeamsFlipped)(void*);
	const Vector& (*GetAbsOrigin)(void*);
	void (*NotifyNetworkStateChanged)(void);
	uint32_t g_iHighestVersusSurvivorCompletion[TEAM_SIZE] = {0};
};


/* Interfaces */
IGameConfig *g_pGameConf = NULL;
ISmmAPI *g_pSmmAPI = NULL;
ICvar *icvar = NULL;
IGameEventManager2 *gameevents = NULL;

SMEXT_LINK(&g_Left4Fix);

bool Left4Fix::SDK_OnLoad(char *error, size_t maxlength, bool late) {
	char conf_error[255] = "";
	void *addr;
	
	if (!gameconfs->LoadGameConfigFile(GAMECONFIG_FILE, &g_pGameConf, conf_error, sizeof(conf_error))) {
		if (conf_error[0]) {
			UTIL_Format(error, maxlength, "Could not read " GAMECONFIG_FILE ".txt: %s", conf_error);
		}
		return false;
	}
	
	/* g_pGameRules *//*
	if(!g_pGameConf->GetMemSig("g_pGameRules", (void **)&addr) || !addr)
	{
	    UTIL_Format(error, maxlength, "Could not read pGameRules signature");
		return false;
	}
	g_pGameRules = reinterpret_cast<void **>(addr);*/
	
	/* g_pDirector */
	if(!g_pGameConf->GetMemSig("g_pDirector", (void **)&addr) || !addr)
	{
	    UTIL_Format(error, maxlength, "Could not read pDirector signature");
		return false;
	}
	g_pDirector = reinterpret_cast<void **>(addr);
	
	if(!g_pGameConf->GetMemSig("CDirector_AreTeamsFlipped", (void **)&Detours::AreTeamsFlipped) || !Detours::AreTeamsFlipped)
	{
	    UTIL_Format(error, maxlength, "Could not read CDirector_AreTeamsFlipped signature");
		return false;
	}
	
	if(!g_pGameConf->GetMemSig("CBaseEntity_GetAbsOrigin", (void **)&Detours::GetAbsOrigin) || !Detours::GetAbsOrigin)
	{
	    UTIL_Format(error, maxlength, "Could not read CBaseEntity_GetAbsOrigin signature");
		return false;
	}
	
	if(!g_pGameConf->GetMemSig("CGameRulesProxy_NotifyNetworkStateChanged", (void **)&Detours::NotifyNetworkStateChanged) || !Detours::NotifyNetworkStateChanged)
	{
	    UTIL_Format(error, maxlength, "Could not read CGameRulesProxy_NotifyNetworkStateChanged signature");
		return false;
	}
	
	playerhelpers->AddClientListener(&g_Left4Fix);
	gameevents->AddListener(&g_OnPlayerDeath, "player_death", true);
	gameevents->AddListener(&g_OnRoundStart,  "round_start",  true);
	
	memset(g_players, 0, sizeof(g_players));
	
	Detour::Init(g_pSM->GetScriptingEngine(), g_pGameConf);
	
	return true;
}

void Left4Fix::SDK_OnAllLoaded() {
	g_PatchManager.Register(new AutoPatch<ScoreCode>());
	g_PatchManager.Register(new AutoPatch<Detours::RecomputeVersusCompletion>());
	g_PatchManager.Register(new AutoPatch<Detours::OnGetCompletionByCharacter>());
	g_PatchManager.Register(new AutoPatch<Detours::RevivedByDefib>());
	g_PatchManager.Register(new AutoPatch<Detours::l4fx_EndVersusModeRound>());
}

/*
inline unsigned int fixMod(unsigned int score) {
	return score - (score % TEAM_SIZE);
}*/

void Left4Fix::OnServerActivated(int max_clients) {
	// while( g_bRoundEnd_Pre ) sleep(1);
	memset(g_iHighestVersusSurvivorCompletion, 0, sizeof(g_iHighestVersusSurvivorCompletion));
	memset(g_players, 0, sizeof(g_players));
	memset(g_scores, 0, sizeof(g_scores));
	__sync_and_and_fetch(&g_totalResult, 0); // g_totalResult = 0;
	
	/**
	* Fix the Max Completion Score
	*/
	
	/*
	while( *g_pGameRules == NULL ) {
		g_pSM->LogError(myself, "` not init, max score not changed.");
		return;
	}
	
	unsigned int *maxCompletion = (unsigned int *)((unsigned char *)(*g_pGameRules) + 3560);
	
	if(*maxCompletion % TEAM_SIZE) {
		g_pSM->LogMessage(myself, "Try patch max score: %d", *maxCompletion);
		unsigned int score = ((*maxCompletion) * TEAM_SIZE) / 8;
		*maxCompletion = fixMod(score);
	}*/
}

void Left4Fix::SDK_OnUnload() {
	g_PatchManager.UnregisterAll();
	gameconfs->CloseGameConfigFile(g_pGameConf);
}

bool Left4Fix::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late) {
	g_pSmmAPI = ismm;
	
	GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, gameevents, IGameEventManager2, INTERFACEVERSION_GAMEEVENTSMANAGER2);
	gpGlobals = g_pSmmAPI->GetCGlobals();
	return true;
}

bool Left4Fix::SDK_OnMetamodUnload(char *error, size_t maxlength) {
	return true;
}
