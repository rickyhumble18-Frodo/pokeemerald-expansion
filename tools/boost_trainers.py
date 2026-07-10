#!/usr/bin/env python3

"""
Usage: python3 tools/boost_trainers.py [--check]

Phase 3a global trainer difficulty pass for the endurance difficulty hack.
Rewrites src/data/trainers.party in place:

  - Excludes bosses: any trainer whose Class is one of Leader, Elite Four,
    Champion, Rival (covers Brendan/May/Wally/Steven), Magma Leader,
    Magma Admin, Aqua Leader or Aqua Admin, plus the empty TRAINER_NONE.
  - Applies a level curve of +20% (ceil), clamped to [5, 255].
  - Fills parties to a minimum of 4 Pokemon (6 for trainers fought inside a
    Hoenn gym, i.e. the trainers you face right before each gym leader).
  - Replaces duplicate species within a party.
  - Padding/replacement species are taken from the wild encounter tables of
    the map the trainer is fought on (level-appropriate and regionally
    available by construction); trainers on maps without wild data fall back
    to species that appear in the wild on Hoenn routes at a level band around
    the padding level.
  - Ensures every modified trainer has at least "AI: Basic Trainer".

Idempotency: each modified trainer gets a marker comment line directly above
its "=== TRAINER_X ===" header (comments inside a trainer block are not
allowed by the cpp+trainerproc pipeline, but between-section comments are
stripped to a harmless blank line). Marked trainers are skipped on re-runs,
so running the script twice produces byte-identical output.

--check only runs the validation pass against the current file.
"""

import hashlib
import json
import math
import pathlib
import re
import sys
from collections import OrderedDict

ROOT = pathlib.Path(__file__).resolve().parent.parent
PARTY_FILE = ROOT / "src/data/trainers.party"
MAPS_DIR = ROOT / "data/maps"
WILD_JSON = ROOT / "src/data/wild_encounters.json"

MARKER = "/* boost_trainers: applied-v1 */"

EXCLUDED_CLASSES = {
    "Leader",
    "Elite Four",
    "Champion",
    "Rival",
    "Magma Leader",
    "Magma Admin",
    "Aqua Leader",
    "Aqua Admin",
}

# Hoenn gym interiors: the trainers fought here are the last ones before each
# gym leader, so they get filled to 6 instead of 4.
GYM_MAP_DIRS = {
    "RustboroCity_Gym",
    "DewfordTown_Gym",
    "MauvilleCity_Gym",
    "LavaridgeTown_Gym_1F",
    "LavaridgeTown_Gym_B1F",
    "PetalburgCity_Gym",
    "FortreeCity_Gym",
    "MossdeepCity_Gym",
    "SootopolisCity_Gym_1F",
    "SootopolisCity_Gym_B1F",
}

LEVEL_MULTIPLIER = 1.2
MIN_LEVEL, MAX_LEVEL = 5, 255
DEFAULT_MIN_PARTY, GYM_MIN_PARTY = 4, 6

SPECIES_LINE_RE = re.compile(
    r"^(?:(?P<nick>[^()@]+?)\s*\((?P<inner>[^()]+)\)|(?P<bare>[^()@]+?))"
    r"\s*(?:\((?P<gender>[MF])\))?\s*(?:@\s*(?P<item>.+?))?\s*$"
)


def normalize_species(name):
    """Poochyena / SPECIES_POOCHYENA / Mr. Mime -> POOCHYENA / MRMIME."""
    name = name.strip()
    if name.upper().startswith("SPECIES_"):
        name = name[len("SPECIES_"):]
    return re.sub(r"[^A-Z0-9]", "", name.upper())


def parse_species_line(line):
    m = SPECIES_LINE_RE.match(line.strip())
    if not m:
        return None, None
    species = m.group("inner") or m.group("bare")
    return species.strip(), m.group("item")


class Mon:
    def __init__(self, lines):
        self.lines = list(lines)

    @property
    def species(self):
        return parse_species_line(self.lines[0])[0]

    @property
    def norm(self):
        return normalize_species(self.species)

    def level(self):
        for ln in self.lines:
            if ln.strip().startswith("Level:"):
                return int(ln.split(":", 1)[1])
        return None

    def set_level(self, level):
        for i, ln in enumerate(self.lines):
            if ln.strip().startswith("Level:"):
                self.lines[i] = f"Level: {level}"
                return
        self.lines.insert(1, f"Level: {level}")

    def replace_species(self, species_const):
        _, item = parse_species_line(self.lines[0])
        first = species_const + (f" @ {item}" if item else "")
        # Drop nickname/gender with the species; drop explicit moves so the
        # replacement falls back to its own level-up moves.
        self.lines = [first] + [
            ln for ln in self.lines[1:] if not ln.strip().startswith("- ")
        ]


class Trainer:
    def __init__(self, header_lines, mons, marked):
        self.header_lines = list(header_lines)  # includes "=== TRAINER_X ==="
        self.mons = mons  # list of Mon
        self.marked = marked
        self.modified = False

    @property
    def tid(self):
        return self.header_lines[0].strip("= ").strip()

    def field(self, name):
        for ln in self.header_lines:
            if ln.startswith(name + ":"):
                return ln.split(":", 1)[1].strip()
        return None

    def has_field(self, name):
        return any(ln.startswith(name + ":") for ln in self.header_lines)

    def add_ai_basic(self):
        if not self.has_field("AI"):
            self.header_lines.append("AI: Basic Trainer")

    def render(self):
        parts = []
        if self.marked:
            parts.append(MARKER)
        parts.extend(self.header_lines)
        for mon in self.mons:
            parts.append("")
            parts.extend(mon.lines)
        return "\n".join(parts)


def parse_party_file(text):
    # Line-anchored: the doc comment at the top of the file contains an
    # indented "=== TRAINER_XXXX ===" example that must stay in the preamble.
    idx = re.search(r"(?m)^=== TRAINER_", text).start()
    preamble = text[:idx]

    sections = []  # (marked, lines)
    pending_marker = False
    for ln in text[idx:].splitlines():
        ln = ln.rstrip()
        if ln == MARKER:
            pending_marker = True
            continue
        if ln.startswith("=== TRAINER_"):
            sections.append((pending_marker, [ln]))
            pending_marker = False
            continue
        if sections:
            sections[-1][1].append(ln)

    trainers = []
    for marked, lines in sections:
        header, mons, cur, in_mons = [], [], [], False
        for ln in lines:
            if not in_mons:
                if ln.strip() == "":
                    in_mons = True
                else:
                    header.append(ln)
                continue
            if ln.strip() == "":
                if cur:
                    mons.append(Mon(cur))
                    cur = []
            else:
                cur.append(ln)
        if cur:
            mons.append(Mon(cur))
        trainers.append(Trainer(header, mons, marked))
    return preamble, trainers


def build_trainer_map_index():
    """trainer id -> map dir, from trainerbattle* script commands."""
    index = {}
    for scripts in sorted(MAPS_DIR.glob("*/scripts.inc")):
        map_dir = scripts.parent.name
        if map_dir.endswith("_Frlg"):
            continue
        for m in re.finditer(r"trainerbattle\w*\s+(TRAINER_\w+)", scripts.read_text()):
            index.setdefault(m.group(1), map_dir)
    return index


def map_dir_to_constant(map_dir):
    map_json = MAPS_DIR / map_dir / "map.json"
    if not map_json.exists():
        return None
    return json.loads(map_json.read_text()).get("id")


def build_wild_pools():
    """map constant -> [(species_const, min_level, max_level)]."""
    data = json.loads(WILD_JSON.read_text())
    group = next(
        g for g in data["wild_encounter_groups"] if g["label"] == "gWildMonHeaders"
    )
    pools = {}
    for enc in group["encounters"]:
        mons = []
        for value in enc.values():
            if isinstance(value, dict) and "mons" in value:
                for mon in value["mons"]:
                    mons.append((mon["species"], mon["min_level"], mon["max_level"]))
        if mons:
            pools.setdefault(enc["map"], []).extend(mons)
    return pools


def hoenn_route_pool(pools):
    pool = []
    for map_const, mons in pools.items():
        if re.fullmatch(r"MAP_ROUTE1[0-3][0-9]", map_const):
            pool.extend(mons)
    return pool


def pick_species(candidates, taken, seed):
    """Deterministically pick a species constant not already in `taken`."""
    options = sorted({sp for sp, _, _ in candidates if normalize_species(sp) not in taken})
    if not options:
        return None
    digest = hashlib.sha1(seed.encode()).digest()
    return options[int.from_bytes(digest[:4], "big") % len(options)]


def level_banded(pool, level, band=8):
    while band <= 64:
        subset = [(sp, lo, hi) for sp, lo, hi in pool if lo - band <= level <= hi + band]
        if subset:
            return subset
        band += 8
    return pool


def boost(trainers, trainer_maps, pools, fallback_pool):
    stats = {
        "modified": 0,
        "skipped_excluded": [],
        "skipped_marked": 0,
        "skipped_empty": [],
        "levels_raised": 0,
        "mons_added": 0,
        "dupes_replaced": 0,
        "ai_added": 0,
        "no_map": [],
    }
    for tr in trainers:
        if tr.tid == "TRAINER_NONE" or not tr.mons:
            if tr.tid != "TRAINER_NONE":
                stats["skipped_empty"].append(tr.tid)
            continue
        if (tr.field("Class") or "") in EXCLUDED_CLASSES:
            stats["skipped_excluded"].append(tr.tid)
            continue
        if tr.marked:
            stats["skipped_marked"] += 1
            continue

        map_dir = trainer_maps.get(tr.tid)
        map_const = map_dir_to_constant(map_dir) if map_dir else None
        local_pool = pools.get(map_const, []) if map_const else []
        if not map_dir:
            stats["no_map"].append(tr.tid)

        # 1. Level curve.
        for mon in tr.mons:
            old = mon.level()
            new = max(MIN_LEVEL, min(MAX_LEVEL, math.ceil(old * LEVEL_MULTIPLIER)))
            if new != old:
                mon.set_level(new)
                stats["levels_raised"] += 1

        # 2. Replace duplicate species.
        seen = set()
        for i, mon in enumerate(tr.mons):
            if mon.norm in seen:
                pool = local_pool or level_banded(fallback_pool, mon.level())
                sp = pick_species(pool, seen, f"{tr.tid}:dedupe:{i}") or pick_species(
                    fallback_pool, seen, f"{tr.tid}:dedupe:{i}"
                )
                if sp:
                    mon.replace_species(sp)
                    stats["dupes_replaced"] += 1
            seen.add(mon.norm)

        # 3. Fill the party.
        target = GYM_MIN_PARTY if map_dir in GYM_MAP_DIRS else DEFAULT_MIN_PARTY
        pad_level = min(mon.level() for mon in tr.mons)
        iv_line = next(
            (ln for ln in tr.mons[0].lines if ln.strip().startswith("IVs:")), None
        )
        slot = 0
        while len(tr.mons) < target:
            taken = {mon.norm for mon in tr.mons}
            pool = local_pool or level_banded(fallback_pool, pad_level)
            sp = pick_species(pool, taken, f"{tr.tid}:pad:{slot}") or pick_species(
                fallback_pool, taken, f"{tr.tid}:pad:{slot}"
            )
            if sp is None:
                break
            lines = [sp, f"Level: {pad_level}"]
            if iv_line:
                lines.append(iv_line.strip())
            tr.mons.append(Mon(lines))
            stats["mons_added"] += 1
            slot += 1

        # 4. AI floor.
        if not tr.has_field("AI"):
            tr.add_ai_basic()
            stats["ai_added"] += 1

        tr.marked = True
        tr.modified = True
        stats["modified"] += 1
    return stats


def validate(trainers):
    errors = []
    for tr in trainers:
        if tr.tid == "TRAINER_NONE":
            continue
        if not tr.mons:
            errors.append(f"{tr.tid}: empty party")
            continue
        if not 1 <= len(tr.mons) <= 6:
            errors.append(f"{tr.tid}: party size {len(tr.mons)}")
        # Excluded bosses keep their vanilla duplicates (e.g. Phoebe's two
        # Dusclops); the no-duplicates rule applies to boosted trainers.
        species = [mon.norm for mon in tr.mons]
        if tr.marked and len(set(species)) != len(species):
            errors.append(f"{tr.tid}: duplicate species {species}")
        for mon in tr.mons:
            lvl = mon.level()
            if lvl is None:
                errors.append(f"{tr.tid}: {mon.species} has no Level line")
            elif tr.marked and not MIN_LEVEL <= lvl <= MAX_LEVEL:
                errors.append(f"{tr.tid}: {mon.species} level {lvl} out of range")
            elif not 1 <= lvl <= MAX_LEVEL:
                errors.append(f"{tr.tid}: {mon.species} level {lvl} out of range")
    return errors


def main():
    text = PARTY_FILE.read_text()
    preamble, trainers = parse_party_file(text)

    if "--check" in sys.argv:
        errors = validate(trainers)
        for e in errors:
            print("ERROR:", e)
        print(f"checked {len(trainers)} trainers, {len(errors)} errors")
        sys.exit(1 if errors else 0)

    trainer_maps = build_trainer_map_index()
    pools = build_wild_pools()
    fallback_pool = hoenn_route_pool(pools)

    stats = boost(trainers, trainer_maps, pools, fallback_pool)

    errors = validate(trainers)
    if errors:
        for e in errors:
            print("ERROR:", e)
        print("Validation failed; not writing output.")
        sys.exit(1)

    out = preamble + "\n\n".join(tr.render() for tr in trainers) + "\n"
    PARTY_FILE.write_text(out)

    print(f"modified:            {stats['modified']}")
    print(f"levels raised:       {stats['levels_raised']}")
    print(f"mons added:          {stats['mons_added']}")
    print(f"duplicates replaced: {stats['dupes_replaced']}")
    print(f"AI lines added:      {stats['ai_added']}")
    print(f"skipped (already boosted): {stats['skipped_marked']}")
    print(f"skipped (excluded bosses): {len(stats['skipped_excluded'])}")
    for tid in stats["skipped_excluded"]:
        print(f"    {tid}")
    if stats["skipped_empty"]:
        print(f"skipped (empty party): {stats['skipped_empty']}")
    if stats["no_map"]:
        print(f"no map found (used level-band fallback pool): {len(stats['no_map'])}")
        for tid in stats["no_map"]:
            print(f"    {tid}")


if __name__ == "__main__":
    main()
