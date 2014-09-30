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

#pragma once

#include "../extension.h"
#include "../codepatch/icodepatch.h"


struct patch_t;


class Detour : public ICodePatch
{
protected: //note: implemented by direct superclass
	static IGameConfig *gameconf;
	static ISourcePawnEngine *spengine;

	unsigned char *signature;
	const char *signatureName;

	unsigned char *trampoline;

	patch_t* m_pRestore;

	//save the trampoline pointer
	virtual void SetTrampoline(void *trampoline) = 0;

	//return a void* to the detour function
	virtual void *GetDetourRaw() = 0;

	virtual void PatchFromSignature(const char *signatureName, void *targetFunction, unsigned char *&originalFunction, unsigned char *&signature);
	virtual void PatchFromAddress(void *targetFunction, unsigned char *&originalFunction, unsigned char *&signature) = 0;

public:
	bool isPatched;

	//Initialize the Detour classes before ever calling Patch()
	static void Init(ISourcePawnEngine *spengine, IGameConfig *gameconf);

	Detour();
	virtual ~Detour();

	// enable the detour logic already implemented
	virtual void Patch();

	// you should implement disable the detour logic by yourself
	virtual void Unpatch() {}

	// get the signature name (i.e. "SpawnTank") from the game conf
	virtual const char *GetSignatureName() = 0;

	virtual unsigned char *GetSignatureAddress() { return nullptr; }

};
