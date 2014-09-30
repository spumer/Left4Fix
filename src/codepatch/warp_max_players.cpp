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

#include "extension.h"
#include "warp_max_players.h"
#include "CDetour/detourhelpers.h"


void WarpMaxPlayers::Patch() {
	if(m_isPatched) return;

	int offset;

	g_pGameConf->GetMemSig("WarpGhost_GetPlayerByCharacter", (void **)&m_pMaxPlayerCount);
	if( !m_pMaxPlayerCount ) {
		g_pSM->LogError(myself, "Can't get the \"WarpMaxPlayers\" signature: %x", m_pMaxPlayerCount);
		return;
	}

	if(g_pGameConf->GetOffset("WarpGhost_MaxPlayerCount", &offset)) {
		m_pMaxPlayerCount += offset;
	}

	SetMemPatchable(m_pMaxPlayerCount, 1);
	*m_pMaxPlayerCount = static_cast<uint8_t>(TEAM_SIZE - 1);

	m_isPatched = true;
}

void WarpMaxPlayers::Unpatch() {
	if(!m_isPatched) return;

	if(m_pMaxPlayerCount) { *m_pMaxPlayerCount = 4; }

	m_isPatched = false;
}