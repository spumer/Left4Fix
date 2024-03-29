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
#include "routine.h"
#include "extension.h"

#define GET_FIRST_EMPTY_ELEMENT(E)	while( E->m_Pos.x ) ++E

void r_nowAlive(const Vector& position, death_info_t *data, size_t len) {
	L4D_DEBUG_LOG("r_nowAlive: %f.%f.%f", position.x, position.y, position.z);
	death_info_t *begin = data;
	death_info_t *nearest = NULL;

	float min_distance = 999999;
	for(; static_cast<size_t>(data - begin) < len; ++data){
		if (data->m_DeathDist == 0) continue;
		float dist = data->m_Pos.DistTo(position);
		L4D_DEBUG_LOG("data->m_Pos.DistTo(position): %f", dist);

		if (dist > min_distance) continue;
		min_distance = dist;
		nearest = data;
	}

	if(nearest) {
		L4D_DEBUG_LOG("RESET SCORE AT: %f.%f.%f", nearest->m_Pos.x, nearest->m_Pos.y, nearest->m_Pos.z);
		memset(nearest, 0, sizeof(death_info_t));
	}

}

void r_nowDead(const Vector& position, uint32_t score, death_info_t *data) {
	L4D_DEBUG_LOG("r_nowDead: %f.%f.%f", position.x, position.y, position.z);
	GET_FIRST_EMPTY_ELEMENT(data);
	data->m_Pos = position;
	data->m_DeathDist = score;
}

int r_appendScores(uint32_t *pCompl, size_t compl_max_inserts, const death_info_t *begin, size_t data_len) {
	death_info_t *data = (death_info_t *)begin;
	int result = 0;
	for(; compl_max_inserts &&
		static_cast<size_t>(data - begin) < data_len; ++data)
	{
		if(data->m_Pos.x) {
			*pCompl++ = data->m_DeathDist;
			result += data->m_DeathDist;
			--compl_max_inserts;
		}
	}
	return result;
}