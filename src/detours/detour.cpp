/**
 * vim: set ts=4 :
 * =============================================================================
 * Left 4 Downtown SourceMod Extension
 * Copyright (C) 2009 Igor "Downtown1" Smirnov.
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
#include "detour.h"
#include "CDetour/detourhelpers.h"



IGameConfig *Detour::gameconf = nullptr;
ISourcePawnEngine *Detour::spengine = nullptr;

void Detour::Init(ISourcePawnEngine *spengine, IGameConfig *gameconf)
{
	Detour::spengine = spengine;
	Detour::gameconf = gameconf;
}

Detour::Detour()
: trampoline(nullptr), m_pRestore(new patch_t()), isPatched(false) {}

Detour::~Detour() {
	delete m_pRestore;
}

void Detour::Patch()
{
	if(isPatched)
	{
		return;
	}

	signatureName = GetSignatureName();
	if(signatureName != nullptr)
	{
		PatchFromSignature(signatureName,  GetDetourRaw(), trampoline, signature);
	}
	else
	{
		signature=GetSignatureAddress();
		if(signature == nullptr)
		{
			g_pSM->LogError(myself, "Detour -- Could not find address for detour");
			return;
		}
		PatchFromAddress(GetDetourRaw(), trampoline, signature);
	}
	SetTrampoline(trampoline);

	isPatched = true;
}


void Detour::PatchFromSignature(const char *signatureName, void *targetFunction, unsigned char *&originalFunction, unsigned char *&signature)
{
	if (!gameconf->GetMemSig(signatureName, (void**)&signature) || !signature)
	{
		g_pSM->LogError(myself, "Detour -- Could not find '%s' signature", signatureName);
		return;
	}
	L4D_DEBUG_LOG("Detour -- beginning patch routine for %s", signatureName);

	PatchFromAddress(targetFunction, originalFunction, signature);
}