modded class SCR_PlayersRestrictionZoneManagerComponent
{
	ref set<SCR_EditorRestrictionZoneEntity> GetZones()
	{
		return m_aRestrictionZones;
	}
	
	override protected void KillPlayerOutOfZone(int playerID, IEntity playerEntity)
	{
		// Reforger Lobby Conflict Edition: never kill or freeze players for being "out of zone" — there is no
		// combat area in the deployment lobby.
		return;
	}
	
	override void SetPlayerZoneData(int playerID, IEntity playerEntity, bool inZone, bool inWarningZone, ERestrictionZoneWarningType warningType, vector zoneCenter = vector.Zero, float warningRadiusSq = -1, float zoneRadiusSq = -1)
	{	
		// Reforger Lobby Conflict Edition: there is no combat-area restriction in the deployment lobby. The lobby
		// camera (InitialPlayer) sits far off-map and deploy spawns may be outside any editor
		// restriction zone, which made the vanilla "Get Back to the battlefield!" warning stick.
		// Always report players as in-zone (no warning, no movement freeze, no out-of-zone kill).
		super.SetPlayerZoneData(playerID, playerEntity, true, false, warningType, zoneCenter, warningRadiusSq, zoneRadiusSq);

		RestrictMovement(playerID, false);
	}
	
	void ResetPlayerZoneData(int playerID)
	{
		SetPlayerZoneData(playerID, null, false, false, -1);
	}
	
	void RestrictMovement(int playerID, bool restrict)
	{
		PlayerController playerController = m_PlayerManager.GetPlayerController(playerID);
		
		if (!playerController)
			return;
		
		PS_PlayableControllerComponent playableControllerComponent = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (!playableControllerComponent)
			return;
		
		playableControllerComponent.SetOutFreezeTime(restrict);
	}
}
