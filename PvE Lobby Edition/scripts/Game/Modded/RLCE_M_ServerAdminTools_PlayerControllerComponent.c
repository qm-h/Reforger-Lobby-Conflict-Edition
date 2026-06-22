// Reforger Lobby Conflict Edition: Server Admin Tools opens its "Server Message" (MOTD) dialog as
// soon as the player controller's RequestServerMotd RPC round-trips. That almost always happens
// BEFORE PS_CoopLobby finishes opening, so the lobby (a MenuBase) is pushed on top of the modal
// MOTD dialog while the dialog keeps keyboard focus -> the welcome screen ends up rendered behind
// the lobby and steals input ("an opening screen that blocks the keys behind my lobby").
//
// We don't touch SAT's data flow; we only defer the actual open until the CoopLobby menu exists,
// so the dialog is the LAST menu pushed -> it sits on top of the lobby and gets focus.
modded class ServerAdminTools_PlayerControllerComponent
{
	override void OpenMotdDialog()
	{
		MenuManager menuManager = GetGame().GetMenuManager();

		// Lobby not open yet: re-arm a single deferred attempt instead of opening behind it.
		if (!menuManager || !menuManager.FindMenuByPreset(ChimeraMenuPreset.CoopLobby))
		{
			GetGame().GetCallqueue().Remove(OpenMotdDialog);
			GetGame().GetCallqueue().CallLater(OpenMotdDialog, 250, false);
			return;
		}

		// Lobby is up: opening now stacks the dialog above it.
		super.OpenMotdDialog();
	}
}
