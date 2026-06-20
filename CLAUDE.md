# CLAUDE.md — PvE Lobby Edition

Arma Reforger mod (Enfusion engine, Enforce Script `.c`). Fork of ReforgerLobby /
PlayableSelector, being reworked into a **persistent 24/7 PvE deployment lobby**.

## ALWAYS read the project intent first

**Before any design or implementation decision, read [`PROJECT.md`](PROJECT.md).**
It defines the goal, the persistent-server model, and the hard constraints. If a
request seems to conflict with `PROJECT.md`, stop and flag it.

The active implementation plan lives in
`docs/superpowers/plans/` — read the latest before touching code.

## Hard rules (do not violate without explicit approval)

1. **Server never ends.** The game mode runs 24/7. Never auto-transition to
   `DEBRIEFING` / `POSTGAME`, never shut the session down, never end on objectives.
   Global state stays in `GAME` permanently.
2. **Play = deploy one player.** The lobby "Play" deploys the *requesting* player
   at their chosen spawn / slot / loadout / group. It must NOT advance global game
   state for everyone.
3. **Objectives are informational.** They can be created/closed by admins in-game
   but must never trigger a state transition or end the match.
4. **Death → back to lobby.** A dead player re-opens the deployment lobby.

## File-naming conventions (respect them)

| Prefix | Meaning | Rule |
|--------|---------|------|
| `O_*`  | **Original vanilla** source, copied in as read-only reference | **NEVER edit.** Read to understand vanilla. Not authoritative for compile. |
| `CEAF_*` | Team modded override / new class | Editable. Our overrides of vanilla Conflict classes. |
| `PS_*` | PlayableSelector mod code (inherited) | Editable. Core mod. |
| `PS_M_SCR_*` | Modded vanilla class (`modded class`) | Editable. Patches a base-game class. |

## Engine / workflow notes

- **No automated test runner.** Verify by: (1) compile in Workbench (no script
  errors), (2) load a test world / dedicated session, (3) observe behavior in-game.
  Plans use "Workbench compile + in-game check" instead of unit tests.
- All vanilla `SCR_*` classes are available at compile (base game loads under the
  mod). `O_*` files are reference only — do not rely on them being in the build.
- Server-authoritative: gate state changes with `Replication.IsServer()`; sync to
  clients via `Rpc` / `[RplProp]` / `Replication.BumpMe()`.
- `PS_PlayableManager` (entity/group based) is the legacy data source. The rework
  rebinds the lobby to `SCR_CampaignFactionManager` + `SCR_GroupPreset` +
  `SCR_LoadoutManager` + `SCR_PlayerSpawnPoint`. Expect coupling work.
- The vanilla respawn/spawnpoint/loadout systems are currently **neutered** by
  `PS_M_SCR_RespawnSystemComponent` (overrides return null/false). Re-enabling them
  is part of the rework — check before assuming they work.

## Communication

- User language: **French**. Reply in French.
- Verify in code before confirming claims about behavior — do not agree blindly.
