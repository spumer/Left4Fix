#include <sourcemod>
#include <sdktools>
#include <left4fix>
#include <left4downtown>

public Plugin:myinfo =
{
	name = "Left4Fix Test",
	author = "spumer",
	description = "",
	version = "1.0",
	url = "https://forums.alliedmods.net/member.php?u=151387"
}

new bool:g_Left4Fix_ext = false;

public OnLibraryAdded(const String:name[]) {
	if(StrEqual(name, "Left4Fix")) {
		g_Left4Fix_ext = true;
	}
}

public OnPluginStart()
{
	RegAdminCmd("sm_flow", Command_GetFlow, ADMFLAG_ROOT);
	g_Left4Fix_ext = LibraryExists("Left4Fix");
}

public Action:Command_GetFlow(client, args) {
	if(!client || !IsClientInGame(client)) return;

	if(!g_Left4Fix_ext) {
		PrintToChat(client, "[L4FIX-Test] Left4Fix extension is not loaded!");
		return;
	}

	if(GetClientTeam(client) != 2) {
		PrintToChat(client, "[L4FIX-Test] You are no in survivors team. Change team and try again.");
		return;
	}

	if(IsClientObserver(client)) {
		PrintToChat(client, "[L4FIX-Test] You are dead. Score can't be obtained.");
		return;
	}

	new client_score = L4FIX_GetSurvivorScore(client);
	if(client_score == -1) {
		PrintToChat(client, "[L4FIX-Test] Score can't be obtained. Sorry.");
		return;
	}

	static team_size = -1;
	if(team_size == -1) {
		team_size = L4FIX_GetSupportedTeamSize();
	}

	new max_score = L4D_GetVersusMaxCompletionScore();
	new Float:max_client_score = FloatDiv(float(max_score), float(team_size));

	PrintToChat(client, "[L4FIX-Test] Your flow is: %.02f%%", FloatDiv(float(client_score), max_client_score) * 100.0);
}