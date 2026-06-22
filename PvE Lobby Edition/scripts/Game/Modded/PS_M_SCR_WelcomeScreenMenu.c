// Reforger Lobby Conflict Edition: the vanilla Conflict welcome/deploy screens are fully replaced by our PS_CoopLobby.
// On a Conflict game mode something (deploy handler, respawn flow, the host's own lobby mod, ...) can
// still open these menus; they render behind our lobby and steal pointer focus, so clicks hit their
// hidden "Continue" button instead of our widgets. As a catch-all that does not depend on WHO opens
// them, make the menus close themselves the moment they open.

modded class SCR_WelcomeScreenMenu : SCR_DeployMenuBase
{
	override void OnMenuOpen()
	{
		if (PS_GameMasterLobbyRoute.TryRouteToLobby(this))
			return;
		super.OnMenuOpen();
		Close();
	}
}

modded class SCR_DeployMenuMain : SCR_DeployMenuBase
{
	override void OnMenuOpen()
	{
		// Persistent PvE: the Game Master "Respawn menu" action opens this vanilla deploy menu
		// (RespawnSuperMenu). We don't use it. A Game Master with no character who presses it wants to
		// drop back to the deployment lobby — close the editor and route the request to the server.
		if (PS_GameMasterLobbyRoute.TryRouteToLobby(this))
			return;
		super.OnMenuOpen();
		Close();
	}
}

// Persistent PvE helper: shared logic so the Game Master "Respawn menu" (which opens a vanilla deploy
// menu we otherwise just close) instead closes the editor. Closing the editor fires
// PS_GameModeCoop.EditorClosed(), which routes an undeployed player into the deployment lobby.
class PS_GameMasterLobbyRoute
{
	static bool TryRouteToLobby(ChimeraMenuBase menu)
	{
		// Only intercept when the LOCAL editor is open in full Game Master (not the limited player editor).
		if (!SCR_EditorManagerEntity.IsOpenedInstance(false))
			return false;

		menu.Close();

		// Close the Game Master editor; EditorClosed() then sends the (now undeployed) player to the lobby.
		Print("[PVE RESPAWN] Game Master respawn menu -> close editor -> lobby", LogLevel.NORMAL);
		SCR_EditorManagerEntity.CloseInstance();

		return true;
	}
}
