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

#include <asm/asm.h>
#include "detour_call.h"
#include "CDetour/detourhelpers.h"


DetourCall::~DetourCall() {}


void DetourCall::PatchFromAddress(void *targetFunction, unsigned char *&originalFunction, unsigned char *&signature) {
	L4D_DEBUG_LOG("DetourCall::PatchFromAddress()");
	originalFunction = signature + OP_CALL_SIZE;
	originalFunction += *(long*)(signature + 1);

	patch_t detourCallPatch;
	detourCallPatch.bytes = OP_CALL_SIZE;
	ReplaceCallAddr(detourCallPatch.patch, signature, targetFunction);

	ApplyPatch(signature, /*offset*/0, &detourCallPatch, m_pRestore);
	L4D_DEBUG_LOG("Detour has been patched for function @ %p", signature);
}

void DetourCall::Unpatch()
{
	//NOTE: careful not to call any vfuncs here since its invoked by destructor

	L4D_DEBUG_LOG("DetourCall::Unpatch()");
	if(!isPatched) return;

	Detour::Unpatch();

	L4D_DEBUG_LOG("DetourCall::Unpatch() -- restoring %s to original state", signatureName);
	ApplyPatch(signature, /*offset*/0, m_pRestore, /*restore*/NULL);

	L4D_DEBUG_LOG("DetourCall %s has been unpatched", signatureName);
	isPatched = false;
}

void DetourCall::ReplaceCallAddr(void* buffer, void* src, void* dest) {
	*(uint8_t*)buffer = *(uint8_t*)src;
	*(long*)((uint8_t*)buffer+1) = (long)((uint8_t*)dest - ((uint8_t*)src + OP_CALL_SIZE));
}