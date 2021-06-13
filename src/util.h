/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Team Fortress 2 Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_UTIL_H_
#define _INCLUDE_UTIL_H_



#define OP_JMP				0xE9
#define OP_JMP_SIZE			5

#define OP_NOP				0x90
#define OP_NOP_SIZE			1

#define OP_PREFIX			0xFF
#define OP_JMP_SEG			0x25

#define OP_JMP_BYTE			0xEB
#define OP_JMP_BYTE_SIZE	2

#define OP_CALL				0xE8
#define OP_CALL_SIZE		5


#define REGISTER_NATIVE_ADDR(name, code) \
	void *addr; \
	if (!g_pGameConf->GetMemSig(name, &addr) || !addr) \
	{ \
		return pContext->ThrowNativeError("Failed to locate function " #name); \
	} \
	code;


size_t UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...);
void memDump(uint8_t *pAddr, size_t len);

// replace address in specific CALL instruction to the given function address
void replace_call_addr(void* src, void* dest);


#endif //_INCLUDE_L4DOWNTOWN_TOOLS_UTIL_H_
