// Reforger Lobby Conflict Edition: vanilla Conflict picks two bases as HQs and randomly assigns BLUFOR/OPFOR to
// them (SetHQFactions -> SetFaction(factionBLUFOR/factionOPFOR) behind a Math.RandomFloat01()
// coin flip), overwriting whatever the mapper authored on each base's SCR_FactionAffiliationComponent.
// That is why a base authored as US can come up USSR on map load: it got selected as an HQ and
// re-factioned at random. A player then deploys from "their" base and lands at an enemy point.
//
// For the persistent PvE lobby we want bases to keep the faction the mapper assigned. We force each
// selected HQ to its authored default faction (GetFaction(true) == the FactionAffiliationComponent
// value). Non-HQ bases already honor their authored faction in InitializeBases() (it only fills in a
// faction when the base has none), so HQs are the only ones that get randomized — and the only ones
// we need to pin here.
//
// Caveat: if two selected HQs were both authored to the same faction, they both stay that faction
// (vanilla would have forced them to opposing sides). For PvE that is intended — enemy presence is
// driven by AI, not by an opposing HQ, and the game never ends on territory (PROJECT.md).
modded class SCR_CampaignMilitaryBaseManager
{
	override void SetHQFactions(notnull array<SCR_CampaignMilitaryBaseComponent> selectedHQs)
	{
		foreach (SCR_CampaignMilitaryBaseComponent hq : selectedHQs)
		{
			if (!hq)
				continue;

			SCR_CampaignFaction authored = SCR_CampaignFaction.Cast(hq.GetFaction(true));
			if (authored)
				hq.SetFaction(authored);
		}
	}
}
