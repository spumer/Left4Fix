#if defined _left4fix_inc
 #endinput
#endif
#define _left4fix_inc

/**
 * @brief	Return the supported team size by this extension instance.
 * For more information read the wiki: https://github.com/spumer/Left4Fix/wiki/How-it-works-inside
 * 
 * @return	TEAM_SIZE value which used for compile this extension
 */
native L4FIX_GetSupportedTeamSize();

/**
 * @brief Get the current client score.
 * @remarks Can be increased or decreased from time to time. Depends on position over map.
 *
 * @param client   survivor, should be alive and in game. Entity reference is supported.
 * @return 	current player score, or -1 if score not found
 */
native L4FIX_GetSurvivorScore(client);


// No dynamic loads, guys. It is dangerously.
/*public Extension:__ext_left4fix = 
{
	name = "Left4Fix",
	file = "left4fix.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};*/
