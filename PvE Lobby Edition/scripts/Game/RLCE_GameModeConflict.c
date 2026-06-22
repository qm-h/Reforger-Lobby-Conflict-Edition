// Reforger Lobby Conflict Edition deployment lobby running on the FULL vanilla Conflict
// (SCR_GameModeCampaign): bases, supplies, AI and capture all stay alive.
//
// PS_GameModeCoop now derives from SCR_GameModeCampaign and carries all the lobby
// logic (group generation, deploy, return-to-lobby) plus the Conflict-end
// neutralization (PROJECT.md: 24/7 server). This thin subclass is the CONCRETE
// class the Conflict game-mode prefab instantiates.
//
// Why a subclass instead of renaming PS_GameModeCoop now: ~50 call sites across the
// mod do PS_GameModeCoop.Cast(GetGame().GetGameMode()). Because RLCE_GameModeConflict
// IS-A PS_GameModeCoop, every one of those casts keeps working unchanged while we
// migrate. The migration plan's phase 6 collapses the two classes into one.
//
// Prefab wiring: point the Conflict game-mode prefab's root entity at
// RLCE_GameModeConflict (it must keep the Conflict components it inherits from
// GameMode_Conflict.et: faction manager, bases system, supply, spawn points, plus our
// overlay components).
class RLCE_GameModeConflictClass : PS_GameModeCoopClass
{
}

class RLCE_GameModeConflict : PS_GameModeCoop
{
}
