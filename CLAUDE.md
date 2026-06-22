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
| `O_*`  | **Original vanilla** source, read-only reference. Lives under `reference/Game/` (NOT `scripts/`) so the engine never compiles these duplicate class defs. | **NEVER edit.** Read to understand vanilla. Not authoritative for compile. |
| `PS_*` | PlayableSelector mod code (inherited) | Editable. Core mod. Keep the `PS_` class names — they are referenced across the inherited codebase, prefabs and layouts; renaming a `PS_` class cascades everywhere. |
| `PS_M_SCR_*` | Modded vanilla class (`modded class`) | Editable. Patches a base-game class. |
| `RLCE_*` | **Reforger Lobby Conflict Edition** — new files authored by this rework. The class inside a modded-class file keeps the vanilla name, so the *file* can safely carry the `RLCE_` prefix. | Editable. Preferred prefix for any new file. |

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

- **Reply in the contributor's own language.** This is an open-source project with
  contributors from many places. Detect the language the developer writes in (issue,
  PR, commit, chat) and answer in that same language. If it is ambiguous, default to
  English.
- Verify in code before confirming claims about behavior — do not agree blindly.

<!-- code-review-graph MCP tools -->
## MCP Tools: code-review-graph

**IMPORTANT: This project has a knowledge graph. ALWAYS use the
code-review-graph MCP tools BEFORE using Grep/Glob/Read to explore
the codebase.** The graph is faster, cheaper (fewer tokens), and gives
you structural context (callers, dependents, test coverage) that file
scanning cannot.

### When to use graph tools FIRST

- **Exploring code**: `semantic_search_nodes` or `query_graph` instead of Grep
- **Understanding impact**: `get_impact_radius` instead of manually tracing imports
- **Code review**: `detect_changes` + `get_review_context` instead of reading entire files
- **Finding relationships**: `query_graph` with callers_of/callees_of/imports_of/tests_for
- **Architecture questions**: `get_architecture_overview` + `list_communities`

Fall back to Grep/Glob/Read **only** when the graph doesn't cover what you need.

### Key Tools

| Tool | Use when |
| ------ | ---------- |
| `detect_changes` | Reviewing code changes — gives risk-scored analysis |
| `get_review_context` | Need source snippets for review — token-efficient |
| `get_impact_radius` | Understanding blast radius of a change |
| `get_affected_flows` | Finding which execution paths are impacted |
| `query_graph` | Tracing callers, callees, imports, tests, dependencies |
| `semantic_search_nodes` | Finding functions/classes by name or keyword |
| `get_architecture_overview` | Understanding high-level codebase structure |
| `refactor_tool` | Planning renames, finding dead code |

### Workflow

1. The graph auto-updates on file changes (via hooks).
2. Use `detect_changes` for code review.
3. Use `get_affected_flows` to understand impact.
4. Use `query_graph` pattern="tests_for" to check coverage.
