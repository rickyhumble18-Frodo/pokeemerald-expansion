# Emerald Endurance — Difficulty Hack Plan

An endurance-focused difficulty hack of Pokémon Emerald, built on
[pokeemerald-expansion](https://github.com/rh-hideout/pokeemerald-expansion)
**1.16.2** (branch created from tag `expansion/1.16.2`, not `master`).

## Concept

A long-haul Emerald where the difficulty curve never flattens: the level
ceiling is raised far past 100, trainers scale up to match, and the Elite Four
becomes a repeatable gauntlet that grows stronger every time you beat it.

## Phases

### Phase 0 — Scaffolding ✅ (done)
- Branch created from `expansion/1.16.2`.
- `.github/workflows/build.yml`: builds the Emerald ROM on every push and on
  manual dispatch (mirrors upstream CI's toolchain: apt `gcc-arm-none-eabi` +
  `make -O all`), verifies `pokeemerald.gba` exists, uploads it as the
  `emerald-difficulty-rom` artifact (14-day retention).
- `tools/generate_exp_tables.py`: generates the full
  `src/data/pokemon/experience_tables.h` for levels 0–255, all six Gen 3
  growth rates. Levels 1–100 use the exact Gen 3 piecewise formulas; levels
  101–255 extend from the level-100 total using medium-fast cubic deltas
  (`exp(n) = exp(100) + (n³ − 100³)`) for **all** growth rates, because
  Erratic's formula goes negative past level 160. Every table is strictly
  increasing and u32-safe. Applied to the codebase in Phase 2.

### Phase 1 — Gameplay configuration ✅ (done)
Decisions recorded below under "Config decisions".
- Modern battle mechanics kept (defaults already Gen 9): physical/special
  split on, Fairy type on.
- Exp. Share behaves as a **held item** (Gen 5 behavior), not modern
  party-wide exp: `I_EXP_SHARE_ITEM` kept at `GEN_5` in
  `include/config/item.h` (this tag's default; pinned with a comment).
- **Perfect IVs for player-obtained Pokémon** (wild catches, gifts, hatched
  eggs): new config flag `P_PERFECT_PLAYER_IVS` in
  `include/config/pokemon.h` (default `TRUE`).
  - `SetBoxMonIVs` (`src/pokemon.c`): the random-IV branch now sets all six
    IVs to 31. Enemy trainer Pokémon are unaffected — trainer parties are
    created via `CreateMon` (no IV roll) and then explicitly overwrite IVs
    from trainer data (`CreateNPCTrainerPartyFromTrainer`,
    `src/battle_main.c`). Frontier/Pyramid/e-Reader mons also set IVs
    explicitly and are unaffected.
  - `InheritIVs` (`src/daycare.c`): skipped under the flag — bred eggs are
    already all-31 at creation; inheriting from (possibly imperfect) parents
    could only lower them.
- Species pool untouched: all species remain available
  (`include/config/species_enabled.h` unmodified).

### Phase 2 — Level cap 255 ✅ (done)
- `MAX_LEVEL` raised to 255 (`include/constants/pokemon.h`).
- `tools/generate_exp_tables.py` output applied to
  `src/data/pokemon/experience_tables.h` (6 growth rates × 256 entries,
  strictly increasing, validated).
- **Save-format changes** (fresh saves only; existing saves incompatible):
  - `PokemonSubstruct0.experience` widened 21 → 25 bits (max table value
    17,221,375 needs 25; 21 held only ~2M). `pokeball`/`nickname11`/
    `nickname12` rearranged within the substruct to free the bits; still
    12 bytes.
  - `PokemonSubstruct3.metLevel` widened 7 → 8 bits; `otGender` moved into
    the former `unused_0B` bit.
- RAM structs: `BattlePokemon.metLevel` and `FormChangeContext.level`
  widened 7 → 8 bits.
- Bug fixed: `TryIncrementMonLevel` used `u8 nextLevel = level + 1`, which
  wraps to 0 at level 255 and would have set the mon's level to 0.
- `sExperienceScalingFactors` (scaled exp formula, Gen 5/7+) extended from
  index 210 to 520 — it is indexed by `faintedLevel * 2 + 10`, which
  reaches 520 at level 255; previously an out-of-bounds read.
- **Obedience decision**: unchanged code. With the Rain Badge (badge 8),
  `GetAttackerObedienceForAction` returns OBEYS unconditionally, so
  badge-complete players get full obedience to 255. Pre-badge-8, the
  vanilla thresholds (10/20/.../80) apply against level met (Gen 8+
  mechanics) — met level now stores the full 0–255 range.
- Caps confirmed off: `B_EXP_CAP_TYPE = EXP_CAP_NONE`,
  `B_LEVEL_CAP_TYPE = LEVEL_CAP_NONE`; `GetCurrentLevelCap()` returns
  `MAX_LEVEL` (255).
- Battle Frontier: Level 50 mode unchanged; Open Level is defined as
  `MAX_LEVEL` upstream, so it now scales to 255 (frontier mon exp is read
  from the extended tables — in range).
- Stat math at 255 verified: max HP 1805, max other stat ~1699 (base 255,
  252 EVs, 31 IVs, boosting nature) — well inside u16. Damage calc's level
  term is 104 (vs 42 at level 100); worst-case base-damage intermediate
  ~9×10⁸ fits u32/s32; scaled-exp math already uses u64.
- Deferred: soft/hard EXP cap gating during the main story (open question).

### Phase 3a — Global trainer difficulty pass ✅ (done)
Trainer data lives in `src/data/trainers.party` (trainerproc "competitive
syntax", piped through `cpp -traditional-cpp` then `tools/trainerproc` into
`src/data/trainers.h` by `trainer_rules.mk`).

`tools/boost_trainers.py` rewrote 760 of 855 trainers:
- Levels +20% (ceil), clamped to [5, 255] — 1458 level lines raised.
- Parties filled to ≥4 Pokémon (≥6 for trainers inside Hoenn gym maps —
  the trainers fought right before each leader); 1720 mons added.
- 123 vanilla duplicate-species slots replaced.
- Padding/replacement species come from the wild encounter tables of the
  map each trainer is fought on (derived from `data/maps/*/scripts.inc`
  `trainerbattle` commands + `src/data/wild_encounters.json`); maps
  without wild data (gyms, hideouts) fall back to Hoenn-route species in
  a level band around the padding level. Padding mons use the party's
  minimum level and vanilla-style 0 IVs.
- 13 trainers with no AI line got `AI: Basic Trainer`.
- Idempotent: boosted trainers carry a `/* boost_trainers: applied-v1 */`
  marker comment above their header (comments between sections are safe in
  the cpp+trainerproc pipeline; inside a section they are not); re-runs are
  byte-identical no-ops. `--check` re-runs validation only.
- Excluded (94): all Leader / Elite Four / Champion / Rival (Brendan, May,
  Wally, Steven) / Magma & Aqua Leader+Admin class trainers, plus the empty
  `TRAINER_NONE`. Frontier brains and apprentice/e-reader-style trainers in
  `trainers.party` (Anabel, Tucker, etc.) were boosted only if their class
  wasn't excluded; FRLG trainers (`trainers_frlg.party`) untouched.
- Validation: 855 trainers checked — party sizes 1–6, no empty parties,
  no duplicate species on modified trainers, all levels in range.

### Phase 3b — Boss overhaul (not started)
- Gym leaders, rivals, Wally, Magma/Aqua bosses, E4 and Champion get
  hand-tuned teams (Phase 4 loops build on the E4 teams).

### Phase 4 — Repeatable Elite Four (not started)
- Elite Four rematches loop indefinitely; each completed loop increments a
  save variable that scales E4/Champion levels upward.
- Builds on Phase 3's trainer data plus a level-offset mechanism keyed to
  that var (candidates: dynamic level scaling at party-creation time in
  `CreateNPCTrainerPartyFromTrainer`, or `B_VAR_DIFFICULTY` trainer tiers).

## Config decisions (2026-07-09)

| Setting | Value | Where | Note |
|---|---|---|---|
| Battle mechanics generation | `GEN_LATEST` = Gen 9 | `include/config/general.h` | Kept — modern mechanics |
| Physical/special split | `B_PHYSICAL_SPECIAL_SPLIT = GEN_LATEST` (on) | `include/config/battle.h` | Kept |
| Fairy type | On (expansion always has it; `P_UPDATED_TYPES = GEN_LATEST` applies retypes) | `include/config/pokemon.h` | Kept |
| Exp. Share | `I_EXP_SHARE_ITEM = GEN_5` (held item), `I_EXP_SHARE_FLAG = 0` (no party-wide toggle) | `include/config/item.h` | Was already the tag default; pinned deliberately |
| Player IVs | `P_PERFECT_PLAYER_IVS = TRUE` (new flag) | `include/config/pokemon.h` | Wild/gift/egg mons get 31s; trainer mons keep trainer-data IVs |
| Level/EXP caps | `EXP_CAP_NONE` / `LEVEL_CAP_NONE` | `include/config/caps.h` | Unchanged for now; revisit in Phase 2 |
| Obedience | `B_OBEDIENCE_MECHANICS = GEN_LATEST` | `include/config/battle.h` | Unchanged; must be audited when raising MAX_LEVEL |
| Trainer AI | Per-trainer flags; presets `AI_FLAG_BASIC_TRAINER` / `AI_FLAG_SMART_TRAINER` (`include/constants/battle_ai.h`); tuning in `include/config/ai.h`; `B_VAR_DIFFICULTY = 0` (off) | — | Unchanged; Phase 3 will raise AI flags per trainer |

## Debug menu (dev builds)

The expansion's built-in debug menus are **enabled in all of our CI/dev
builds**. `include/config/debug.h` gates them on `DISABLED_ON_RELEASE`,
which is `TRUE` for any build that doesn't define `RELEASE` — and both our
local builds and CI use `make all`, not `make release`. Verified present in
the built ROM (`Debug_ShowMainMenu` linked at `0x0810a370`).

- **Overworld debug menu**: hold **R**, then press **START**
  (`DEBUG_OVERWORLD_MENU` / `DEBUG_OVERWORLD_HELD_KEYS = R_BUTTON`).
- **Battle debug menu**: press **SELECT** during a battle
  (`DEBUG_BATTLE_MENU`).
- Bonus: Pokémon sprite visualizer via **SELECT** on the summary screen.

⚠️ **Release builds must disable this later**: build final ROMs with
`make release`, which defines `RELEASE` → `NDEBUG` and flips every
`DISABLED_ON_RELEASE` config (debug menus, AGBPrint) off automatically. Do
not ship a `make all` ROM.

## Open questions

- **Species pool**: currently every species is enabled
  (`include/config/species_enabled.h` defaults). Decide whether the hack
  restricts the pool (e.g. Hoenn-only, no legendaries pre-E4) or stays fully
  open. Left fully open for now.
- Whether Phase 2 uses a soft/hard EXP cap during the main story or leaves
  leveling completely free.
- How E4 loop scaling interacts with the 255 cap (linear per loop? capped?).
