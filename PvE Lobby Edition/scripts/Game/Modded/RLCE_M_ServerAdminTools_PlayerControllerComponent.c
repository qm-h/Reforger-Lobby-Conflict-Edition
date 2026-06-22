// Reforger Lobby Conflict Edition: Server Admin Tools opens its "Server Message" (MOTD) dialog as
// soon as the player controller's RequestServerMotd RPC round-trips. On a real/dedicated server
// (SAT does NOT run in Workbench solo play, which is why this is invisible there) that modal dialog
// pops on top of PS_CoopLobby and keeps keyboard/mouse focus -> the lobby is rendered but blocked,
// which also makes the post-death return to lobby look broken (the lobby IS open, just unreachable
// behind the dialog).
//
// The lobby IS our front-end (PROJECT.md), so the MOTD has no place over it: suppress the dialog
// entirely. We only neuter the UI pop-up; SAT's underlying data/RPC flow is untouched.
modded class ServerAdminTools_PlayerControllerComponent
{
	override void OpenMotdDialog()
	{
		// Persistent PvE lobby: never open the SAT MOTD dialog (it stole focus over CoopLobby).
		return;
	}
}
