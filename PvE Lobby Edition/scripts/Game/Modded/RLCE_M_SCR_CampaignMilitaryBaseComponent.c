// Reforger Lobby Conflict Edition: on a full Conflict, every SCR_CampaignMilitaryBase subscribes to the
// "local player faction assigned" event and rebuilds its map descriptor from that faction
// (OnLocalPlayerFactionAssigned -> m_MapDescriptor.MapSetup). In the lobby the local player
// has no faction yet (faction key ""), so the event can fire with a NULL faction and the
// vanilla body dereferences it ("NULL pointer to instance, assignedFaction"). Guard the null
// case; the descriptor will be set up properly once the player actually picks a faction.
modded class SCR_CampaignMilitaryBaseComponent
{
	override protected void OnLocalPlayerFactionAssigned(Faction assignedFaction)
	{
		if (!assignedFaction)
			return;

		super.OnLocalPlayerFactionAssigned(assignedFaction);
	}
}
