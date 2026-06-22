modded class SCR_PlayerDeployMenuHandlerComponent : ScriptComponent
{
	// Reforger Lobby Conflict Edition: the vanilla OnPostInit wires the Conflict deploy flow by grabbing the player
	// controller's SCR_RespawnComponent and subscribing to its m_OnRespawnReadyInvoker_O. On our
	// lobby controller that respawn wiring is neutered/absent, so the vanilla body dereferences a
	// NULL respawn component and throws ("NULL pointer to instance, m_OnRespawnReadyInvoker_O").
	// We don't want the vanilla deploy/welcome flow at all (PS_CoopLobby replaces it), so skip the
	// vanilla setup entirely and keep only the FRAME tick that drives our Update() menu-closer.
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.FRAME);
	}

	// Reforger Lobby Conflict Edition: our PS_CoopLobby fully replaces the vanilla Conflict deploy/welcome screen.
	// On a Conflict game mode the player controller carries this handler; its base Update() opens
	// ChimeraMenuPreset.WelcomeScreenMenu and the RespawnSuperMenu (SCR_DeployMenuMain) whenever the
	// player has no deployed character. Those vanilla menus render BEHIND our lobby but keep input
	// focus, so clicks land on the hidden deploy menu instead of our widgets (the lobby looks
	// unresponsive). The original PlayableSelector never hit this because it runs on its own game
	// mode, not Conflict. Forbid the handler from ever opening them.
	override protected bool CanOpenMenu()
	{
		return false;
	}

	override protected bool CanOpenWelcomeScreen()
	{
		return false;
	}

	// Belt-and-suspenders: also actively close anything that slipped through on this frame.
	override void Update(float timeSlice)
	{
		MenuManager menuManager = GetGame().GetMenuManager();
		if (menuManager)
			menuManager.CloseMenuByPreset(ChimeraMenuPreset.WelcomeScreenMenu);

		if (SCR_DeployMenuMain.GetDeployMenu())
			SCR_DeployMenuMain.CloseDeployMenu();
	}
}
