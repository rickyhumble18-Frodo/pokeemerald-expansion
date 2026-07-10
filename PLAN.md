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

### Phase 3b — Hand-authored boss fights ✅ (done)
`tools/boss_teams.py` holds curated teams for every boss and splices them
into `src/data/trainers.party` (56 authored teams + 36 derived rematches =
92 sections; idempotent — output depends only on the data file).

- **Coverage**: all 8 gym leaders (story fights; `_2`–`_5` rematches derive
  from the authored team at +6/+12/+18/+24 levels), every May/Brendan fight
  (all 3 starter variants × 5 stages, ramping 3→4→5→6→6 mons), Wally
  (Mauville 3 mons; Victory Road 6, rematches +4 steps), all Magma/Aqua
  admin+leader fights (no Courtney battle exists in Emerald), Elite Four,
  Champion Wallace, and the Steven superboss. The two unused
  `*_PLACEHOLDER` rival entries were left alone.
- **Every boss mon**: 31 IVs in all six stats, set-appropriate nature,
  held item, EV spread, and a curated 4-move set. All data emitted as
  `SPECIES_/ITEM_/MOVE_/NATURE_/ABILITY_` constants; abilities verified
  legal against `species_info` (runtime assert would fire otherwise).
- **AI**: every boss has `Smart Trainer / Ace Pokemon / Hp Aware`; top
  fights (E4, Wallace, Steven, VR Wally, Archie, late rivals) add
  `Prediction`, per upstream's `docs/tutorials/ai_flags.md` guidance that
  `AI_FLAG_PREDICTION` pairs with `AI_FLAG_SMART_TRAINER`.
- **Levels**: ~3–5 above the Phase 3a curve at each location (e.g. Roxanne
  14–17 over gym trainers at 12; E4 57–62 + Wallace to 65 over Victory
  Road's 54). **E4/Wallace teams are the loop-0 baselines Phase 4 scales
  from.**
- **Later-gen picks** (flagged): Roxanne Larvitar (G2), Brawly Timburr
  (G5), Wattson Luxio (G4), Flannery Houndour (G2), Tate&Liza Bronzong
  (G4, Trick Room), Juan Politoed (G2, Drizzle rain captain), Phoebe
  Mismagius+Dusknoir (G4), Glacia Abomasnow+Froslass (G4, hail core),
  Drake Garchomp (G4), Maxie/Tabitha Houndoom (G2). Everything else is
  Hoenn-dex native.

**Tuning principle**: the player is underleveled but has perfect IVs;
bosses compensate with full teams, perfect IVs, held items, EVs and smart
AI — never pure level inflation.

### Phase 4 — Repeatable, scaling Elite Four ✅ (done)

**Route decision**: evaluated (a) the expansion's `B_VAR_DIFFICULTY` tier
system vs (b) a custom hook in the trainer party loader. **Chose (b).**
The tier system (`include/constants/difficulty.h`, `src/difficulty.c`)
supports exactly three fixed tiers (Easy/Normal/Hard), each requiring a
separate hand-written party in `trainers.party` — it cannot express
unbounded "+5 per loop" scaling, and maintaining N copies of five boss
teams per tier would be miserable. The hook is ~40 lines, scales
indefinitely, and keeps Phase 3b's authored teams as the single source of
truth.

- `VAR_ELITE_FOUR_LOOPS` (repurposed `VAR_UNUSED_0x404E`), incremented by
  `addvar` in the Hall of Fame map script — inside the `VAR_TEMP_1`-gated
  entry cutscene, immediately before the game-clear flags call, so it
  fires exactly once per clear.
- **Level scaling**: `ApplyEliteFourLoopLevelBoost` in `src/battle_main.c`
  runs at the end of `CreateNPCTrainerParty` for
  Sidney/Phoebe/Glacia/Drake/Wallace: each mon's level += 5 × loops,
  clamped at 255, applied by writing the exp-table value and calling
  `CalculateMonStats` (level is derived from EXP there). **Movesets are
  the preset Phase 3b 4-move sets stored in trainer data — they are
  assigned independently of level and are identical at every loop**, so
  nothing regenerates or breaks as levels scale.
- **Re-enterability & room reset**: vanilla Emerald already handles this —
  `EverGrandeCity_HallOfFame_EventScript_ResetEliteFour` (called from the
  game-clear flags script) clears all four `FLAG_DEFEATED_ELITE_4_*` and
  zeroes `VAR_ELITE_4_STATE`, so doors, NPCs and rooms reset for a fresh
  run every time. Verified; no changes needed.
- **Credits skip**: `StartCredits` (`src/hall_of_fame.c`) checks the loop
  var (incremented before `GameClear` runs, so it reads 1 on the first
  clear): loops ≥ 2 skips the credits roll and continues straight from
  the Hall of Fame save via `CB2_ContinueSavedGame`.
- **Lobby NPC**: a "loop scholar" (`LOCALID_LEAGUE_LOOP_SCHOLAR`, Expert
  graphics) in the League 1F lobby states the loop count and the current
  E4 level range via the new `BufferEliteFourLoopStats` special
  (`src/field_specials.c`, registered in `data/specials.inc`).
- **Prize money**: `GetTrainerMoneyToGive` reads the *static* trainer-data
  level, so the level boost alone wouldn't raise payouts — E4/Champion
  rewards are multiplied by `loops + 1` (clamped by `AddMoney`'s
  `MAX_MONEY` on receipt).
- **Test plan** (debug menu → Flags/Vars → `VAR_ELITE_FOUR_LOOPS`):
  | loops | E4/Champion levels |
  |---|---|
  | 0 | baseline 57–65 (Sidney 57–59 … Wallace 62–65) |
  | 1 | 62–70 |
  | 5 | 82–90 |
  | 20 | 157–165 |
  Clamp: Wallace's ace hits 255 at loop 38; everything is 255 by loop 40.
  The increment cannot double-fire: the Hall of Fame cutscene is gated on
  `VAR_TEMP_1 == 0`, sets it to 1 mid-script, and the player is warped out
  of the map afterward.

### Phase 5 — Endurance quality-of-life ✅ (done)
- **Nature sage** (League 1F lobby, ¥5,000): reuses the expansion's mint
  infrastructure — the hidden-nature system (`MON_DATA_HIDDEN_NATURE`) and
  the existing `SetHiddenNature` special (sets nature + recalculates stats
  immediately). Mint items exist and work too (`ItemUseCB_Mint`), but the
  NPC flow was requested: pick a party mon (`ChoosePartyMon`, egg-guarded),
  pick the stat to raise and the stat to lower from two 5-entry menus
  (new `MULTI_NATURE_STAT_UP/DOWN` lists; nature = up×5 + down, verified
  against the `gNaturesInfo` table — picking the same stat twice gives the
  matching neutral nature), confirm with the nature name buffered by the
  new `ComputeNatureFromStatChoices` special, pay, done.
- **League quartermaster** (League 1F lobby): scripted vendor on the
  Frontier scrollable-multichoice pattern (`SCROLL_MULTI_LEAGUE_QUARTERMASTER`)
  with custom steep prices, since pokemarts can't override item prices:
  Full Restore ¥9,800 · Max Revive ¥24,000 · Max Elixir ¥12,000 · all six
  X items / Guard Spec. / Dire Hit ¥3,000 · Rare Candy ¥40,000 · Lucky Egg
  ¥100,000. Economy check: a loop-1 League run pays roughly ¥40–60k
  (base payouts × loops+1), so one run buys ~1 Rare Candy or a few
  restores — meaningful but not trivial, and prices stay relevant as loop
  income grows linearly.
- PLAN sweep: no debug stock or temporary changes were ever added to shops
  (the Slateport Rare Candy idea never materialized); the only intentional
  dev-only feature is the debug menu, which stays ON in dev/CI builds and
  is auto-disabled by `make release` (see "Debug menu" section).

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

## Resolved design decisions (formerly open questions)

- **Species pool**: stays fully open (`species_enabled.h` defaults). The
  difficulty model is "underleveled player vs stacked bosses", which works
  regardless of what the player catches; restricting the pool would fight
  the endurance concept, not support it.
- **EXP caps**: none. Leveling stays completely free
  (`EXP_CAP_NONE`/`LEVEL_CAP_NONE`); the E4 loop treadmill outpaces
  grinding anyway, and the 255 ceiling is the only hard limit.
- **E4 loop scaling**: resolved in Phase 4 — linear +5 per loop from the
  Phase 3b baselines, clamped at 255 (ace caps at loop 38, whole League
  by loop 40). Prize money scales ×(loops+1).
