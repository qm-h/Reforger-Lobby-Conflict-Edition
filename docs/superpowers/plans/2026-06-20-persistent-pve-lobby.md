# Persistent PvE Lobby — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: use superpowers:subagent-driven-development
> (recommended) or superpowers:executing-plans to implement this plan task-by-task.
> Steps use checkbox (`- [ ]`) syntax for tracking.
>
> **READ FIRST:** [`PROJECT.md`](../../../PROJECT.md) and [`CLAUDE.md`](../../../CLAUDE.md).

**Goal:** Convert the PlayableSelector lobby into a persistent 24/7 PvE deployment
system where "Play" deploys a single player (faction → group preset → role → loadout
→ faction spawn point), keeping the mod's existing lobby UI and never ending the match.

**Architecture:** Two decoupled efforts. (A) **Lifecycle neutralization** — freeze the
game mode in `GAME`, make Play a per-player deploy, kill debriefing/shutdown. (B) **Data
rebind** — feed the existing lobby UI from Conflict systems (`SCR_CampaignFactionManager`,
`SCR_GroupPreset`, `SCR_LoadoutManager`, `SCR_PlayerSpawnPoint`) instead of hand-placed
playable entities, and re-enable the neutered vanilla respawn/spawn-point/loadout stack.

**Tech Stack:** Enfusion engine, Enforce Script (`.c`), Reforger Conflict/Campaign
framework. Server-authoritative replication (`Replication.IsServer()`, `Rpc`, `[RplProp]`).

## Global Constraints

- Server runs 24/7; global game state stays in `GAME`. Never reach `DEBRIEFING`/`POSTGAME`. (PROJECT.md)
- "Play" deploys only the requesting player; it must not advance global state. (PROJECT.md)
- Objectives are informational; closing one never changes state or ends the match. (PROJECT.md)
- Death returns the player to the deployment lobby. (PROJECT.md)
- Briefing kept as optional non-blocking info screen; debriefing removed. (PROJECT.md)
- No automated tests. Each task verifies via **Workbench compile (zero script errors)**
  + **in-game observation** on a local/dedicated test world.
- Never edit `O_*` files (read-only vanilla reference). Edit `CEAF_*`, `PS_*`, `PS_M_SCR_*`.
- Server-gate every state mutation; replicate to clients.

> **Path note:** all source paths below are relative to
> `PvE Lobby Edition/scripts/Game/` unless stated otherwise.

---

## Phase A — Persistent server lifecycle

Lowest-risk, independent of the data rebind. Delivers the 24/7 behavior first.

### Task A1: Freeze the game-mode state machine in `GAME`

**Files:**
- Modify: `PS_GameModeCoop.c` — `AdvanceGameState()` (~lines 878-917), `StartGame()` (~919).

**Interfaces:**
- Produces: a game mode that, once it reaches `GAME`, never auto-advances to
  `DEBRIEFING`/`POSTGAME`. `StartGame()` semantics unchanged for entering `GAME`.

- [ ] **Step 1: Read current state machine**
  Read `PS_GameModeCoop.c:840-960` to confirm exact case labels and `SetGameModeState`
  signature before editing.

- [ ] **Step 2: Remove the terminal transitions**
  In `AdvanceGameState`, delete/no-op the `GAME → DEBRIEFING` and
  `DEBRIEFING → POSTGAME` cases so the switch cannot leave `GAME`:
  ```c
  case SCR_EGameModeState.GAME:
      // Persistent server: never auto-end. Stay in GAME.
      break;
  case SCR_EGameModeState.DEBRIEFING:  // unreachable, kept for enum completeness
      break;
  ```

- [ ] **Step 3: Grep for other end-of-match callers**
  Run from `scripts/Game/`:
  `grep -rniE "DEBRIEFING|POSTGAME|EndGameMode|EndSession|EndGame\b" .`
  Neutralize any code path (timers, objective handlers, score listeners) that calls
  `SetGameModeState(SCR_EGameModeState.DEBRIEFING/POSTGAME)` or session shutdown.
  Record each hit and its disposition in the task notes.

- [ ] **Step 4: Workbench compile**
  Open the project in Workbench. Expected: **0 script compile errors**.

- [ ] **Step 5: In-game verification**
  Launch a local test world. Reach `GAME`. Confirm: the match does not advance to
  debriefing on its own; no auto server stop. Let it idle several minutes.

- [ ] **Step 6: Commit**
  ```bash
  git add "PvE Lobby Edition/scripts/Game/PS_GameModeCoop.c"
  git commit -m "feat(lifecycle): freeze game mode in GAME, remove debriefing/postgame transitions"
  ```

### Task A2: Make "Play" a per-player deploy, not a global advance

**Files:**
- Modify: `UI/GameMode/PS_GameModeHeader.c` — `Action_Advance()`.
- Read: `Playable/PS_PlayableControllerComponent.c` (where `AdvanceGameState` is defined)
  to add a per-player deploy RPC.

**Interfaces:**
- Consumes: lobby selection state (chosen group/role/spawn/loadout) — provided by Phase B.
  Until Phase B exists, deploy uses the current playable-selection path.
- Produces: `PS_PlayableControllerComponent.RequestDeploy()` (server RPC) that spawns the
  requesting player only.

- [ ] **Step 1: Read the Play handler and controller**
  Read `PS_GameModeHeader.c` `Action_Advance` and the `AdvanceGameState` RPC in
  `PS_PlayableControllerComponent.c`. Confirm the player-id plumbing.

- [ ] **Step 2: Add a per-player deploy RPC on the controller**
  In `PS_PlayableControllerComponent.c`, add a server RPC that deploys only the calling
  player using their current lobby selection (do not call `AdvanceGameState`):
  ```c
  void RequestDeploy() { Rpc(RPC_RequestDeploy); }
  [RplRpc(RplChannel.Reliable, RplRcver.Server)]
  void RPC_RequestDeploy()
  {
      // server-only: spawn THIS player at their selected spawn/slot/loadout.
      // Phase B fills the spawn/loadout source; for now reuse existing selection spawn.
  }
  ```

- [ ] **Step 3: Repoint the Play button**
  In `PS_GameModeHeader.c`, change `Action_Advance` so that when state is `GAME`
  (the permanent state) it calls `playableController.RequestDeploy()` instead of being
  inert / advancing state:
  ```c
  void Action_Advance(SCR_ButtonBaseComponent button)
  {
      PS_PlayableControllerComponent playableController = ...; // unchanged lookup
      playableController.RequestDeploy();
  }
  ```

- [ ] **Step 4: Workbench compile** — expected 0 errors.

- [ ] **Step 5: In-game verification**
  Two-client test: client 1 presses Play and deploys; client 2 stays in lobby and is
  unaffected (global state unchanged, client 2 not spawned).

- [ ] **Step 6: Commit**
  ```bash
  git add "PvE Lobby Edition/scripts/Game/UI/GameMode/PS_GameModeHeader.c" \
          "PvE Lobby Edition/scripts/Game/Playable/PS_PlayableControllerComponent.c"
  git commit -m "feat(deploy): Play deploys requesting player only, no global state advance"
  ```

### Task A3: Death returns the player to the deployment lobby

**Files:**
- Modify: `Playable/PS_PlayableComponent.c` — `OnDamageStateChange()` / `TryRespawn()` (~212-228).
- Modify: `PS_GameModeCoop.c` — `TryRespawn()` path if it forces mission respawn.

**Interfaces:**
- Consumes: `PS_PlayableControllerComponent.SwitchToMenu(...)` (existing) to open the lobby.
- Produces: on death, the dead player's client re-opens the deployment lobby.

- [ ] **Step 1: Read death path**
  Read `PS_PlayableComponent.c:212-228` and `PS_GameModeCoop.TryRespawn` to confirm
  current behavior.

- [ ] **Step 2: Redirect death to lobby**
  On `EDamageState.DESTROYED`, instead of mission respawn, open the lobby for that player
  (server tells the owning client to `SwitchToMenu` the deployment lobby). Keep it
  server-gated and addressed to the dead player only.

- [ ] **Step 3: Workbench compile** — expected 0 errors.

- [ ] **Step 4: In-game verification**
  Deploy, die. Confirm the deployment lobby re-opens for the dead player and they can
  redeploy. Other players unaffected.

- [ ] **Step 5: Commit**
  ```bash
  git add "PvE Lobby Edition/scripts/Game/Playable/PS_PlayableComponent.c" \
          "PvE Lobby Edition/scripts/Game/PS_GameModeCoop.c"
  git commit -m "feat(respawn): dead player returns to deployment lobby"
  ```

### Task A4: Briefing optional, debriefing removed from UI/nav

**Files:**
- Modify: `UI/GameMode/PS_GameModeHeader.c` — briefing/lobby nav actions.
- Inspect: `UI/Debriefing/*` — disable/route-around debriefing menu opening.

**Interfaces:**
- Produces: no UI path opens a debriefing menu; briefing remains reachable but non-blocking.

- [ ] **Step 1: Find debriefing openers**
  Run `grep -rniE "Debriefing|DEBRIEFING" .` and list every menu-open / state-check.

- [ ] **Step 2: Remove debriefing openers**
  Disable the calls that open the debriefing menu or gate UI on `DEBRIEFING`. Leave the
  briefing screen reachable from the header as an info view.

- [ ] **Step 3: Workbench compile** — expected 0 errors.

- [ ] **Step 4: In-game verification**
  Confirm briefing still opens as info; no debriefing screen ever appears.

- [ ] **Step 5: Commit**
  ```bash
  git add "PvE Lobby Edition/scripts/Game/UI/"
  git commit -m "feat(ui): briefing optional info screen, debriefing removed"
  ```

---

## Phase B — Conflict data rebind (faction / group / role / loadout / spawn)

Replaces the placed-entity data source behind the lobby with Conflict systems and
re-enables the neutered vanilla stack. Each task reads the matching `O_*` reference for
exact signatures.

### Task B1: Re-enable the vanilla respawn/spawn-point stack

**Files:**
- Modify: `GameMode/Respawn/PS_M_SCR_RespawnSystemComponent.c` (currently all overrides
  return `null`/`false`).
- Read: `GameMode/Respawn/O_SCR_PlayerSpawnPointManagerComponent.c`,
  `GameMode/Respawn/O_SCR_PlayerSpawnPoint.c`,
  `Respawn/Loadouts/O_SCR_PlayerLoadoutComponent.c`.

**Interfaces:**
- Produces: a working respawn system instance + faction spawn points usable by deploy.

- [ ] **Step 1: Read the `O_*` respawn references** to learn the real method signatures
  the modded class must restore (`GetInstance`, `IsRespawnEnabled`,
  `ServerSetEnableRespawn`, `GetRplComponent`, spawn-point request flow).

- [ ] **Step 2: Restore the overrides**
  Replace the stubbed returns with real delegation to the base implementation (call
  `super`), so spawn points and the respawn manager function again. Keep any mod-specific
  behavior the original intended, documented inline.

- [ ] **Step 3: Workbench compile** — expected 0 errors.

- [ ] **Step 4: In-game verification**
  Confirm `SCR_PlayerSpawnPoint`s placed for a faction are discoverable
  (`SCR_PlayerSpawnPointManagerComponent.GetInstance()` non-null, spawn points listed).

- [ ] **Step 5: Commit**
  ```bash
  git add "PvE Lobby Edition/scripts/Game/GameMode/Respawn/PS_M_SCR_RespawnSystemComponent.c"
  git commit -m "feat(respawn): re-enable vanilla respawn/spawn-point stack"
  ```

### Task B2: Faction list from `SCR_CampaignFactionManager`

**Files:**
- Modify: `UI/Lobby/PS_CoopLobby.c` — faction list population (`UpdatePlayerFaction`, faction list build).
- Read: `Faction/O_SCR_CampaignFaction.c`, `GameMode/FactionManager/O_SCR_CampaignFactionManager.c`,
  the `CEAF_*` equivalents.

**Interfaces:**
- Consumes: `SCR_CampaignFactionManager.GetCampaignFactionByIndex/Key`, `GetFactionsList`.
- Produces: lobby faction list sourced from the faction manager (playable factions only),
  replacing the `PS_PlayableManager`-derived faction set.

- [ ] **Step 1: Read** `PS_CoopLobby.c` faction-list build + the faction manager API.

- [ ] **Step 2: Swap the faction source**
  Build the faction list from `SCR_CampaignFactionManager.GetFactionsList()` filtered by
  `IsPlayable()`, instead of factions inferred from placed playables.

- [ ] **Step 3: Workbench compile** — expected 0 errors.

- [ ] **Step 4: In-game verification**
  Lobby shows exactly the playable campaign factions (e.g. US / USSR / FIA), no phantom
  factions from missing placed entities.

- [ ] **Step 5: Commit**
  ```bash
  git add "PvE Lobby Edition/scripts/Game/UI/Lobby/PS_CoopLobby.c"
  git commit -m "feat(lobby): factions sourced from SCR_CampaignFactionManager"
  ```

### Task B3: Group + role lists from `SCR_GroupPreset` / `SCR_GroupRolePresetConfig`

**Files:**
- Modify: `UI/Lobby/PS_CoopLobby.c` — group/role list (the CharactersList/GroupList build).
- Read: `Groups/O_SCR_GroupPreset.c`, `Groups/O_SCR_GroupRolePresetConfig.c`,
  `Groups/O_SCR_GroupsManagerComponent.c`.

**Interfaces:**
- Consumes: `SCR_GroupsManagerComponent.GetInstance()`, preset/role config accessors,
  `SCR_EGroupRole`.
- Produces: lobby group list = predefined presets for the selected faction; expanding a
  group shows its role slots from the role preset config.

- [ ] **Step 1: Read** the lobby group/role build and the three `O_*` group references.

- [ ] **Step 2: Swap the group/role source**
  For the selected faction, list `SCR_GroupPreset`s; per group, list role slots from
  `SCR_GroupRolePresetConfig` (with occupancy counts), replacing the per-entity
  `PS_PlayableComponent` rows.

- [ ] **Step 3: Workbench compile** — expected 0 errors.

- [ ] **Step 4: In-game verification**
  Selecting a faction lists its predefined groups; expanding shows roles with free/taken
  slot counts (mirrors the vanilla Deployment Setup screens).

- [ ] **Step 5: Commit**
  ```bash
  git add "PvE Lobby Edition/scripts/Game/UI/Lobby/PS_CoopLobby.c"
  git commit -m "feat(lobby): groups/roles sourced from SCR_GroupPreset"
  ```

### Task B4: Loadout selection + preview via `SCR_LoadoutManager`

**Files:**
- Modify: `UI/Lobby/PS_LobbyLoadoutPreview.c` (existing preview), `UI/Lobby/PS_CoopLobby.c`.
- Read: `GameMode/Loadout/O_SCR_LoadoutManager.c`, `O_SCR_PlayerFactionLoadout.c`,
  `O_SCR_PlayerArsenalLoadout.c`, `UI/Menu/GameMode/O_SCR_LoadoutPreviewComponent.c`.

**Interfaces:**
- Consumes: `SCR_LoadoutManager` loadout list per faction/role, `SCR_LoadoutPreviewComponent`.
- Produces: a chosen loadout stored in the player's lobby selection, shown in preview.

- [ ] **Step 1: Read** the loadout manager + preview references.

- [ ] **Step 2: Wire loadout list + preview**
  Populate available loadouts for the selected role/faction from `SCR_LoadoutManager`;
  drive `PS_LobbyLoadoutPreview` with the chosen loadout via the preview component.

- [ ] **Step 3: Workbench compile** — expected 0 errors.

- [ ] **Step 4: In-game verification**
  Selecting a role lists loadouts; preview updates; selection persists into deploy.

- [ ] **Step 5: Commit**
  ```bash
  git add "PvE Lobby Edition/scripts/Game/UI/Lobby/"
  git commit -m "feat(lobby): loadout selection + preview via SCR_LoadoutManager"
  ```

### Task B5: Spawn-point selection on map + deploy wiring

**Files:**
- Modify: `UI/Lobby/PS_CoopLobby.c` (or its map sub-panel), `PS_PlayableControllerComponent.c`
  (`RPC_RequestDeploy` from Task A2).
- Read: `GameMode/Respawn/O_SCR_PlayerSpawnPoint.c`,
  `GameMode/Respawn/O_SCR_PlayerSpawnPointManagerComponent.c`.

**Interfaces:**
- Consumes: `SCR_PlayerSpawnPointManagerComponent` faction spawn points; lobby selection
  (faction/group/role/loadout from B2–B4).
- Produces: `RPC_RequestDeploy` spawns the player at the chosen `SCR_PlayerSpawnPoint`
  with the chosen loadout, joined to the chosen group.

- [ ] **Step 1: Read** the spawn-point references and the existing map panel in the lobby.

- [ ] **Step 2: List + pick spawn points**
  Show the selected faction's `SCR_PlayerSpawnPoint`s on the lobby map; store the picked
  one in the player's lobby selection.

- [ ] **Step 3: Implement deploy**
  Fill `RPC_RequestDeploy` (Task A2) to: validate selection, spawn the player at the
  chosen spawn point, apply the chosen loadout, join the chosen group/role. Server-gated.

- [ ] **Step 4: Workbench compile** — expected 0 errors.

- [ ] **Step 5: In-game verification**
  Full flow: faction → group → role → loadout → map spawn point → Play → player spawns at
  that point with that loadout, in that group. Die → lobby → redeploy elsewhere.

- [ ] **Step 6: Commit**
  ```bash
  git add "PvE Lobby Edition/scripts/Game/UI/Lobby/" \
          "PvE Lobby Edition/scripts/Game/Playable/PS_PlayableControllerComponent.c"
  git commit -m "feat(deploy): map spawn-point selection + full deploy wiring"
  ```

---

## Phase C — Objectives (informational only)

### Task C1: Admin-managed objectives that never end the match

**Files:**
- Inspect: `scripts/Game/ObjectiveSystem/*` (if present) or add a minimal objective holder.

**Interfaces:**
- Produces: create/close objective actions for admins; zero coupling to game state.

- [ ] **Step 1: Audit** existing objective code:
  `grep -rniE "objective" scripts/Game/`. Confirm none calls `SetGameModeState` or session end.

- [ ] **Step 2: Add/confirm admin create+close**
  Ensure objectives can be created and closed in-game by admins (`PS_PlayersHelper.IsAdminOrServer()`),
  with no state transition on completion.

- [ ] **Step 3: Workbench compile** — expected 0 errors.

- [ ] **Step 4: In-game verification**
  Admin creates an objective, closes it; match continues, server stays up, state stays `GAME`.

- [ ] **Step 5: Commit**
  ```bash
  git add "PvE Lobby Edition/scripts/Game/"
  git commit -m "feat(objectives): admin-managed, informational only, never ends match"
  ```

---

## Self-review notes

- **Spec coverage:** persistent server (A1), per-player deploy (A2, B5), death→lobby (A3),
  briefing optional / no debriefing (A4), faction (B2), group/role (B3), loadout (B4),
  spawn point (B5), objectives informational (C1). All PROJECT.md requirements mapped.
- **Sequencing:** Phase A delivers the 24/7 behavior on the existing data path; Phase B
  rebinds data and depends on A2's `RPC_RequestDeploy` and B1's re-enabled respawn stack;
  B5 closes the loop. C is independent.
- **Open reads (do at execution time, not now):** exact signatures in the `O_*` files for
  loadout/group/spawn APIs — each task's Step 1 reads them before editing. This is
  deliberate: the `O_*` reference is the source of truth for vanilla signatures, not memory.
