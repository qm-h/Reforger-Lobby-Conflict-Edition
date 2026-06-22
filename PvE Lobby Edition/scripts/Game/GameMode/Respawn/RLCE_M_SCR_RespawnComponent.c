// Reforger Lobby Conflict Edition: the pause-menu "Respawn" entry calls SCR_RespawnComponent.RequestPlayerSuicide()
// on the local player controller. The vanilla pipeline (kill -> deploy menu) is neutered for our
// 24/7 lobby, so a plain suicide left the player stuck ("Respawn -> Confirm does nothing"). Route
// the request to the server, which kills the deployed body and re-opens the deployment lobby
// (PS_GameModeCoop.ReturnPlayerToLobby), exactly like dying does (PROJECT.md #4).
modded class SCR_RespawnComponent : RespawnComponent
{
	override void RequestPlayerSuicide()
	{
		Print("[PVE RESPAWN] RequestPlayerSuicide() called (pause-menu respawn confirm)", LogLevel.NORMAL);
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (playerController)
		{
			PS_PlayableControllerComponent playableController = playerController.PS_GetPLayableComponent();
			if (playableController)
			{
				Print("[PVE RESPAWN] -> PS_RequestReturnToLobby (playableController OK)", LogLevel.NORMAL);
				playableController.PS_RequestReturnToLobby();
				return;
			}
			Print("[PVE RESPAWN] no PS_PlayableControllerComponent on controller -> vanilla suicide", LogLevel.WARNING);
		}
		else
		{
			Print("[PVE RESPAWN] no local player controller -> vanilla suicide", LogLevel.WARNING);
		}

		// Fallback: no lobby controller (e.g. admin/spectator) -> vanilla behaviour.
		super.RequestPlayerSuicide();
	}
}
