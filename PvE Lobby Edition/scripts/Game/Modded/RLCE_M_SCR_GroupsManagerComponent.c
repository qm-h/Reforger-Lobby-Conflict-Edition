modded class SCR_GroupsManagerComponent : SCR_BaseGameModeComponent
{
	// Reforger Lobby Conflict Edition hardening: the vanilla GetFirstNotFullForFaction iterates the faction's
	// playable groups WITHOUT a null check (unlike its sibling helpers), so a stale/null group
	// entry causes a "NULL pointer to instance" VM exception. This is hit whenever a player's
	// faction changes (OnPlayerFactionChanged) — including our deploy flow via ApplyPlayable.
	// We re-implement it with the missing null guard.
	override SCR_AIGroup GetFirstNotFullForFaction(notnull Faction faction, SCR_AIGroup ownGroup = null, bool respectPrivate = false)
	{
		SCR_AIGroup group;
		array<SCR_AIGroup> factionGroups = GetPlayableGroupsByFaction(faction);
		if (!factionGroups)
			return group;

		for (int i = 0, count = factionGroups.Count(); i < count; i++)
		{
			SCR_AIGroup current = factionGroups[i];
			if (!current)
				continue;
			if (!current.IsFull() && current != ownGroup && (!respectPrivate || !current.IsPrivate()))
			{
				group = current;
				break;
			}
		}

		return group;
	}
}
