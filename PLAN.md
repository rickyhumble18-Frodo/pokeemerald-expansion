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
  increasing and u32-safe. Validated; **not yet applied** to the codebase.

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

### Phase 2 — Level cap 255 (not started)
- Apply `tools/generate_exp_tables.py` output to
  `src/data/pokemon/experience_tables.h`.
- Raise `MAX_LEVEL` to 255 and audit everything that assumes 100
  (obedience, badge scaling, summary screen, level-up moves, Rare Candy, ...).
- Consider `include/config/caps.h` (`B_EXP_CAP_TYPE`/`B_LEVEL_CAP_TYPE`,
  currently both NONE) for progression gating during the main game.

### Phase 3 — Trainer overhaul (not started)
- All trainers get fuller teams and higher levels across the whole game.
- Likely tooling: bulk edits to `src/data/trainers.party` (trainerproc
  format); possibly the expansion's `B_VAR_DIFFICULTY` system for
  difficulty-tiered trainer variants.

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

## Open questions

- **Species pool**: currently every species is enabled
  (`include/config/species_enabled.h` defaults). Decide whether the hack
  restricts the pool (e.g. Hoenn-only, no legendaries pre-E4) or stays fully
  open. Left fully open for now.
- Whether Phase 2 uses a soft/hard EXP cap during the main story or leaves
  leveling completely free.
- How E4 loop scaling interacts with the 255 cap (linear per loop? capped?).
