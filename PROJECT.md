# PvE Lobby Edition — Project Intent

## One-line goal

Turn the old-Arma-style **PlayableSelector lobby** into a **persistent 24/7 PvE
deployment system** driven by Conflict/Campaign factions, group presets, loadouts
and faction spawn points — with **no mission lifecycle**.

## Where it comes from

Fork of **ReforgerLobby / PlayableSelector** (by JiraF4). The original brings back
the classic Arma lobby: you pick a pre-placed **playable character** (an entity
hand-placed in the editor, inside an `SCR_AIGroup`); the faction/role/squad lists
are built from those placed entities; pressing Play starts the mission and you
spawn exactly where the entity was placed. The match runs A→B: briefing → game →
debriefing → server stops once objectives are met.

**We do not want that lifecycle.**

## Target model

### Data source: Conflict, not placed entities
The lobby keeps the original UI (`PS_CoopLobby` / `PS_PlayableRespawnMenu`), but
the data behind it changes:

| Lobby element | Old source (PlayableSelector) | New source (this project) |
|---------------|-------------------------------|---------------------------|
| Faction       | `FactionAffiliationComponent` of placed char | `SCR_CampaignFactionManager` |
| Group/squad   | `SCR_AIGroup` placed in editor | `SCR_GroupPreset` / `SCR_GroupsManagerComponent` |
| Slot/role     | placed `PS_PlayableComponent`  | `SCR_GroupRolePresetConfig` |
| Loadout       | none (char = fixed prefab)     | `SCR_LoadoutManager` (+ preview) |
| Spawn point   | exact transform of placed char | `SCR_PlayerSpawnPoint` (faction deploy points on map) |

Reference vanilla source is provided as read-only `O_*` files under
`PvE Lobby Edition/reference/Game/` (mirrors the `scripts/Game/` tree; **not
compiled** — kept out of `scripts/` to avoid duplicate-class errors).

### Lifecycle: persistent server (24/7)
- The mission **runs forever**. The server never shuts down on its own.
- The global game state stays in **`GAME` permanently**. No automatic
  `BRIEFING → GAME → DEBRIEFING → POSTGAME` progression.
- **"Play" deploys only the requesting player** — it spawns *that* player at the
  spawn point / slot / loadout / group they chose. It does not advance global state.
- On **death**, the player returns to the deployment lobby and redeploys.
- **Briefing** stays as an optional info screen (current objectives). It blocks
  nothing and triggers nothing.
- **Debriefing is removed.**

### Objectives
Objectives can be **defined and closed in-game by responsible players/admins**.
Completing or closing an objective **never** ends the match, never changes game
state, never stops the server. They are purely informational/operational.

## Decisions locked (2026-06-20)

- UI approach: **keep the mod's lobby UI** (not the vanilla Deployment Setup menu).
- Global state: **stay in `GAME` permanently**.
- Briefing: **kept, optional, non-blocking**. Debriefing: **removed**.
- Death: **return to lobby / redeploy**.

## Out of scope (for now)

- Replacing the lobby UI with the vanilla `SCR_RespawnMenu`.
- Mission objective *scoring* that affects match outcome.
- Anything that can stop the dedicated session.

## Key files to know

- `scripts/Game/PS_GameModeCoop.c` — game-mode state machine (`AdvanceGameState`,
  `StartGame`). The lifecycle to neutralize lives here.
- `scripts/Game/UI/GameMode/PS_GameModeHeader.c` — the "Play" button
  (`Action_Advance`) and lobby/briefing nav.
- `scripts/Game/UI/Lobby/PS_CoopLobby.c` — lobby UI, currently bound to
  `PS_PlayableManager`.
- `scripts/Game/GameMode/Respawn/PS_M_SCR_RespawnSystemComponent.c` — vanilla
  respawn system, currently neutered.
- `reference/Game/.../O_*` — read-only vanilla reference (loadout, groups, faction,
  spawn point). Mirrors `scripts/Game/` paths but lives under `reference/` so the
  engine does not compile the duplicate class definitions.
