#!/usr/bin/env python3
"""Build and conservatively audit gameid.txt from maintained PS2 catalogues."""

from __future__ import annotations

import argparse
import ast
from collections import Counter, defaultdict
from dataclasses import dataclass
import json
from pathlib import Path
import re
import subprocess
import sys
import unicodedata


GAME_ID_RE = re.compile(r"^[A-Z0-9]{4}_[A-Z0-9]{3}\.[A-Z0-9]{2}$")
SERIAL_RE = re.compile(r"^([A-Z0-9]{4})[-_]([A-Z0-9]{3})[.\-]?([A-Z0-9]{2})$")
OFFICIAL_SERIAL_RE = re.compile(r"^([A-Z0-9]{4})[-_](\d{3})[.\-]?(\d{2})$")
PCSX2_RECORD_RE = re.compile(r"^([A-Z0-9]{4}-[A-Z0-9]{5}):(?:\s+#.*)?$")
PCSX2_FIELD_RE = re.compile(r"^\s{2}(name|name-en):\s*(.*?)\s*$")
WINDOWS_FORBIDDEN_RE = re.compile(r'[<>:"/\\|?*]')
WINDOWS_RESERVED = {
    "CON", "PRN", "AUX", "NUL",
    *(f"COM{number}" for number in range(1, 10)),
    *(f"LPT{number}" for number in range(1, 10)),
}
SOURCE_ORDER = ("pcsx2", "gamedb", "gdx")
SOURCE_LABELS = {
    "gamedb": "GameDB-PS2",
    "gdx": "GDX English TitlesDB",
    "pcsx2": "PCSX2 GameIndex",
    "get_config": "Get_CONFIG PS2 index",
}


@dataclass(frozen=True)
class Change:
    action: str
    game_id: str
    old_title: str
    new_title: str
    evidence: str
    reason: str


@dataclass(frozen=True)
class Review:
    game_id: str
    current_title: str
    gamedb_title: str
    gdx_title: str
    pcsx2_title: str
    get_config_title: str
    reason: str


def canonical_game_id(value: str) -> str | None:
    value = value.strip().upper()
    if GAME_ID_RE.fullmatch(value):
        return value
    match = SERIAL_RE.fullmatch(value)
    if not match:
        return None
    return f"{match.group(1)}_{match.group(2)}.{match.group(3)}"


def canonical_official_id(value: str) -> str | None:
    match = OFFICIAL_SERIAL_RE.fullmatch(value.strip().upper())
    if not match:
        return None
    return f"{match.group(1)}_{match.group(2)}.{match.group(3)}"


def decode_storage_title(title: str) -> str:
    return title.replace("^!", "!").strip(" \t\r\n\0")


def filename_safe_title(title: str) -> str:
    """Normalize catalogue punctuation without emitting an invalid filename."""
    title = unicodedata.normalize("NFC", decode_storage_title(title))
    title = title.replace("\u2013", " - ").replace("\u2014", " - ")
    title = re.sub(r"\s*[:/\\|]+\s*", " - ", title)
    title = title.replace('"', "'")
    title = re.sub(r"[<>?*]", "", title)
    title = re.sub(r"\s+-\s+-\s+", " - ", title)
    title = re.sub(r"\s+", " ", title).strip(" .")
    if not title:
        raise ValueError("title is empty after filename-safety normalization")
    stem = title.split(".", 1)[0].upper()
    if stem in WINDOWS_RESERVED:
        title = "_" + title
    if WINDOWS_FORBIDDEN_RE.search(title) or title.endswith((" ", ".")):
        raise ValueError(f"title remains unsafe for Windows: {title!r}")
    return title


def storage_title(title: str) -> str:
    return filename_safe_title(title).replace("!", "^!")


def title_key(title: str) -> str:
    decomposed = unicodedata.normalize("NFKD", decode_storage_title(title)).casefold()
    return "".join(character for character in decomposed
                   if unicodedata.category(character) != "Mn" and character.isalnum())


def parse_base(path: Path) -> tuple[dict[str, str], list[tuple[str, str]], list[str]]:
    records: dict[str, str] = {}
    canonicalizations: list[tuple[str, str]] = []
    duplicate_ids: list[str] = []
    for line_number, raw_line in enumerate(path.read_text(encoding="utf-8-sig").splitlines(), 1):
        line = raw_line.rstrip(" \t\r\n\0")
        if not line:
            continue
        try:
            raw_id, raw_title = line.split(" ", 1)
        except ValueError as error:
            raise ValueError(f"{path}:{line_number}: missing title separator") from error
        game_id = canonical_game_id(raw_id)
        if game_id is None:
            raise ValueError(f"{path}:{line_number}: invalid game ID {raw_id!r}")
        if game_id != raw_id:
            canonicalizations.append((raw_id, game_id))
        title = filename_safe_title(raw_title)
        if game_id in records:
            duplicate_ids.append(game_id)
            continue
        records[game_id] = title
    if not records:
        raise ValueError(f"{path}: no records")
    return records, canonicalizations, duplicate_ids


def parse_gamedb(root: Path) -> dict[str, str]:
    games = root / "games" if (root / "games").is_dir() else root
    records: dict[str, str] = {}
    for title_path in sorted(games.glob("*/title.txt")):
        game_id = canonical_official_id(title_path.parent.name)
        if game_id is None:
            continue
        lines = title_path.read_text(encoding="utf-8-sig", errors="replace").splitlines()
        if lines and lines[0].strip():
            records.setdefault(game_id, filename_safe_title(lines[0]))
    if not records:
        raise ValueError(f"{root}: no GameDB records")
    return records


def parse_gdx(path: Path) -> tuple[dict[str, str], dict[str, list[str]]]:
    grouped: dict[str, list[str]] = defaultdict(list)
    for raw_line in path.read_text(encoding="utf-8-sig", errors="replace").splitlines():
        if " " not in raw_line:
            continue
        raw_id, raw_title = raw_line.split(" ", 1)
        game_id = canonical_official_id(raw_id)
        if game_id is None or not raw_title.strip():
            continue
        title = filename_safe_title(raw_title)
        if title not in grouped[game_id]:
            grouped[game_id].append(title)
    conflicts = {game_id: titles for game_id, titles in grouped.items() if len(titles) > 1}
    grouped.pop("SLUS_206.43", None)
    records = {game_id: titles[0] for game_id, titles in grouped.items()}
    if not records:
        raise ValueError(f"{path}: no GDX records")
    return records, conflicts


def parse_yaml_scalar(value: str) -> str:
    value = value.strip()
    if value.startswith(('"', "'")):
        try:
            parsed = ast.literal_eval(value)
            return str(parsed)
        except (SyntaxError, ValueError):
            return value.strip('"\'')
    return value.split(" #", 1)[0].strip()


def parse_pcsx2(path: Path) -> dict[str, str]:
    records: dict[str, str] = {}
    current_id: str | None = None
    current_name = ""
    current_english = ""

    def store_current() -> None:
        if current_id and (current_english or current_name):
            records[current_id] = filename_safe_title(current_english or current_name)

    for raw_line in path.read_text(encoding="utf-8-sig", errors="replace").splitlines():
        record_match = PCSX2_RECORD_RE.fullmatch(raw_line)
        if record_match:
            store_current()
            current_id = canonical_official_id(record_match.group(1))
            current_name = ""
            current_english = ""
            continue
        if current_id is None:
            continue
        field_match = PCSX2_FIELD_RE.fullmatch(raw_line)
        if not field_match:
            continue
        title = parse_yaml_scalar(field_match.group(2))
        if field_match.group(1) == "name-en":
            current_english = title
        else:
            current_name = title
    store_current()
    if not records:
        raise ValueError(f"{path}: no PCSX2 records")
    return records


def parse_get_config(path: Path) -> dict[str, str]:
    records: dict[str, str] = {}
    for raw_line in path.read_text(encoding="utf-8-sig", errors="replace").splitlines():
        if "\t" not in raw_line:
            continue
        raw_id, raw_title = raw_line.split("\t", 1)
        game_id = canonical_official_id(raw_id)
        if game_id is not None and raw_title.strip():
            records.setdefault(game_id, filename_safe_title(raw_title))
    if not records:
        raise ValueError(f"{path}: no Get_CONFIG records")
    return records


def parse_tsv(path: Path, expected_header: tuple[str, ...]) -> list[dict[str, str]]:
    lines = path.read_text(encoding="utf-8-sig").splitlines()
    if not lines:
        raise ValueError(f"{path}: empty manifest")
    header = tuple(lines[0].split("\t"))
    if header != expected_header:
        raise ValueError(f"{path}: unexpected header")
    rows: list[dict[str, str]] = []
    for line_number, line in enumerate(lines[1:], 2):
        if not line.strip():
            continue
        values = line.split("\t")
        if len(values) != len(header):
            raise ValueError(f"{path}:{line_number}: expected {len(header)} columns")
        rows.append(dict(zip(header, values)))
    return rows


def reviewed_overrides(path: Path) -> tuple[dict[str, str], dict[str, str]]:
    titles: dict[str, str] = {}
    evidence: dict[str, str] = {}
    for row in parse_tsv(path, ("game_id", "title", "evidence", "note")):
        game_id = canonical_game_id(row["game_id"])
        if game_id is None or game_id != row["game_id"]:
            raise ValueError(f"{path}: non-canonical reviewed ID {row['game_id']!r}")
        if game_id in titles:
            raise ValueError(f"{path}: duplicate reviewed ID {game_id}")
        titles[game_id] = filename_safe_title(row["title"])
        evidence[game_id] = row["evidence"]
    return titles, evidence


def translation_overrides(path: Path) -> tuple[dict[str, str], dict[str, str], dict[str, str]]:
    titles: dict[str, str] = {}
    evidence: dict[str, str] = {}
    retail_ids: dict[str, str] = {}
    for row in parse_tsv(path, ("patched_game_id", "retail_game_id", "title", "evidence", "note")):
        patched = canonical_game_id(row["patched_game_id"])
        retail = canonical_game_id(row["retail_game_id"])
        if patched is None or retail is None or patched == retail:
            raise ValueError(f"{path}: translation must have distinct canonical patched/retail IDs")
        if patched in titles:
            raise ValueError(f"{path}: duplicate patched ID {patched}")
        title = filename_safe_title(row["title"])
        if not title.endswith("[English Translation]"):
            raise ValueError(f"{path}: translated title must end in [English Translation]")
        titles[patched] = title
        evidence[patched] = row["evidence"]
        retail_ids[patched] = retail
    return titles, evidence, retail_ids


def git_revision(path: Path) -> str:
    candidate = path if path.is_dir() else path.parent
    try:
        return subprocess.run(
            ["git", "-C", str(candidate), "rev-parse", "HEAD"],
            check=True, capture_output=True, text=True).stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        return "unknown"


def reference_titles(game_id: str, sources: dict[str, dict[str, str]]) -> dict[str, str]:
    return {name: records[game_id] for name, records in sources.items() if game_id in records}


def select_reference(refs: dict[str, str]) -> tuple[str, str, int]:
    votes: dict[str, list[str]] = defaultdict(list)
    for source, title in refs.items():
        votes[title_key(title)].append(source)
    winning_key, voters = sorted(
        votes.items(),
        key=lambda item: (-len(item[1]),
                          min(SOURCE_ORDER.index(source) for source in item[1]),
                          item[0]))[0]
    for source in SOURCE_ORDER:
        if source in voters:
            return refs[source], winning_key, len(voters)
    raise AssertionError("reference vote has no source")


def is_placeholder(title: str) -> bool:
    key = title_key(title)
    return key.startswith("zzzunknown") or key in {"unknown", "unknowntitle", "tbd"}


def false_prefix_counterpart(game_id: str) -> str | None:
    prefix, suffix = game_id.split("_", 1)
    counterpart = {"SCES": "SLES", "SLES": "SCES", "SCEE": "SLES"}.get(prefix)
    return f"{counterpart}_{suffix}" if counterpart else None


def format_refs(refs: dict[str, str]) -> str:
    return "; ".join(f"{SOURCE_LABELS[name]}: {title}" for name, title in refs.items())


def artifact_label(path: Path, repository_root: Path) -> str:
    try:
        return path.resolve().relative_to(repository_root.resolve()).as_posix()
    except ValueError:
        return path.as_posix()


def audit(args: argparse.Namespace) -> dict[str, int]:
    release_label = args.release_version.removeprefix("v").removeprefix("V")
    if not re.fullmatch(r"\d+\.\d+\.\d+", release_label):
        raise ValueError(f"invalid release version: {args.release_version!r}")

    base, canonicalizations, base_duplicates = parse_base(args.base)
    gamedb = parse_gamedb(args.gamedb)
    gdx, gdx_conflicts = parse_gdx(args.gdx)
    pcsx2 = parse_pcsx2(args.pcsx2)
    get_config = parse_get_config(args.get_config)
    reviewed, reviewed_evidence = reviewed_overrides(args.reviewed)
    translated, translation_evidence, retail_ids = translation_overrides(args.translations)
    overlap = set(reviewed) & set(translated)
    if overlap:
        raise ValueError(f"IDs occur in both override manifests: {', '.join(sorted(overlap))}")

    sources = {"gamedb": gamedb, "gdx": gdx, "pcsx2": pcsx2}
    minimum_source_sizes = {"gamedb": 12000, "gdx": 11000, "pcsx2": 12000,
                            "get_config": 10000}
    parsed_sizes = {name: len(records) for name, records in sources.items()}
    parsed_sizes["get_config"] = len(get_config)
    for source, minimum in minimum_source_sizes.items():
        if parsed_sizes[source] < minimum:
            raise ValueError(f"{SOURCE_LABELS[source]} appears incomplete: "
                             f"{parsed_sizes[source]} IDs (minimum {minimum})")

    primary_ids = set().union(*(set(records) for records in sources.values()))
    support_ids = set(get_config)
    protected_ids = set(reviewed) | set(translated)
    changes: list[Change] = []
    reviews: dict[str, Review] = {}
    final: dict[str, str] = {}

    for old_id, game_id in canonicalizations:
        changes.append(Change("canonicalized-id", game_id, old_id, game_id,
                              "Base database parser",
                              "Normalized the historical hyphen separator to AAAA_BBB.CC."))
    for game_id in base_duplicates:
        changes.append(Change("removed-duplicate", game_id, base[game_id], base[game_id],
                              "Base database parser", "Removed a later duplicate; first record retained."))

    for game_id, current_title in sorted(base.items()):
        counterpart = false_prefix_counterpart(game_id)
        if (game_id not in primary_ids and game_id not in support_ids and
                game_id not in protected_ids and counterpart is not None and counterpart in base):
            counterpart_refs = reference_titles(counterpart, sources)
            evidence = (format_refs(counterpart_refs) if counterpart_refs else
                        f"Base counterpart: {counterpart} {base[counterpart]}")
            changes.append(Change("removed-prefix-shadow", game_id, current_title, "", evidence,
                                  f"Unsupported {game_id[:4]} alias shadows existing counterpart {counterpart}."))
            continue

        refs = reference_titles(game_id, sources)
        chosen = current_title
        action = ""
        reason = ""
        evidence = ""

        if game_id in reviewed:
            chosen = reviewed[game_id]
            action = "reviewed-correction"
            reason = "Applied the human-reviewed title ledger."
            evidence = reviewed_evidence[game_id]
        elif game_id in translated:
            chosen = translated[game_id]
            action = "verified-translation"
            reason = ("This unofficial patch changes the executable ID; the distinct retail-ID record "
                      "remains unlabelled.")
            evidence = translation_evidence[game_id]
        elif refs:
            reference, winning_key, agreement = select_reference(refs)
            current_key = title_key(current_title)
            all_three_agree = len(refs) == 3 and agreement == 3
            if is_placeholder(current_title):
                chosen = reference
                action = "placeholder-replacement"
                reason = f"Replaced a placeholder using {agreement}-source best evidence."
                evidence = format_refs(refs)
            elif (agreement >= 2 and current_key != winning_key and len(current_key) >= 5 and
                  current_key in winning_key):
                chosen = reference
                action = "abbreviation-expansion"
                reason = f"Expanded an incomplete title supported by {agreement} maintained catalogues."
                evidence = format_refs(refs)
            elif all_three_agree and current_key != winning_key:
                chosen = reference
                action = "three-source-correction"
                reason = "All three primary catalogues agree on a different title."
                evidence = format_refs(refs)
            elif current_key != winning_key:
                reason_text = ("Current title is richer than the consensus; retained for manual review."
                               if winning_key in current_key and len(winning_key) >= 5 else
                               "Catalogue titles differ and the conservative policy retained the current title.")
                reviews[game_id] = Review(
                    game_id, current_title, gamedb.get(game_id, ""), gdx.get(game_id, ""),
                    pcsx2.get(game_id, ""), get_config.get(game_id, ""), reason_text)
        elif game_id not in protected_ids:
            reason_text = ("Present only in the support index; no primary-catalogue title vote."
                           if game_id in support_ids else
                           "Not present in any of the four reference snapshots; retained as a legacy/custom ID.")
            reviews[game_id] = Review(
                game_id, current_title, "", "", "", get_config.get(game_id, ""), reason_text)

        chosen = filename_safe_title(chosen)
        final[game_id] = chosen
        if chosen != current_title:
            changes.append(Change(action or "title-correction", game_id, current_title, chosen,
                                  evidence, reason))

    for game_id in sorted(primary_ids - set(final)):
        refs = reference_titles(game_id, sources)
        chosen, _, agreement = select_reference(refs)
        chosen = filename_safe_title(chosen)
        final[game_id] = chosen
        changes.append(Change("added-primary-record", game_id, "", chosen, format_refs(refs),
                              f"Added missing ID from primary catalogue data ({agreement}-source title agreement)."))
        if agreement < 2 or len({title_key(title) for title in refs.values()}) > 1:
            reviews[game_id] = Review(
                game_id, chosen, gamedb.get(game_id, ""), gdx.get(game_id, ""),
                pcsx2.get(game_id, ""), get_config.get(game_id, ""),
                "New record lacks a two-source title consensus; selected by documented source priority.")

    for game_id, title in {**reviewed, **translated}.items():
        if game_id in final:
            continue
        final[game_id] = title
        evidence = reviewed_evidence.get(game_id, translation_evidence.get(game_id, ""))
        action = "reviewed-addition" if game_id in reviewed else "verified-translation-addition"
        changes.append(Change(action, game_id, "", title, evidence, "Added from a verified manifest."))

    for patched_id, retail_id in retail_ids.items():
        if patched_id not in final or retail_id not in final:
            raise ValueError(f"translation pair is incomplete: {patched_id} / {retail_id}")
        if "[English Translation]" in final[retail_id]:
            raise ValueError(f"retail ID {retail_id} must remain unlabelled")

    if len(final) != len(set(final)):
        raise AssertionError("final database contains duplicate IDs")
    for game_id, title in final.items():
        if not GAME_ID_RE.fullmatch(game_id):
            raise AssertionError(f"non-canonical final ID: {game_id}")
        filename_safe_title(title)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", encoding="utf-8", newline="") as output_file:
        for game_id, title in sorted(final.items()):
            output_file.write(f"{game_id} {storage_title(title)}\r\n")

    args.changes.parent.mkdir(parents=True, exist_ok=True)
    with args.changes.open("w", encoding="utf-8", newline="") as change_file:
        change_file.write("action\tgame_id\told_title\tnew_title\tevidence\treason\n")
        for change in sorted(changes, key=lambda item: (item.game_id, item.action)):
            values = (change.action, change.game_id, change.old_title, change.new_title,
                      change.evidence, change.reason)
            change_file.write("\t".join(value.replace("\t", " ").replace("\n", " ")
                                        for value in values) + "\n")

    for game_id, titles in gdx_conflicts.items():
        if game_id not in reviews:
            reviews[game_id] = Review(
                game_id, final.get(game_id, ""), gamedb.get(game_id, ""), " | ".join(titles),
                pcsx2.get(game_id, ""), get_config.get(game_id, ""),
                "GDX assigns more than one title to this ID; inspect the edition/disc conflict.")

    args.review_queue.parent.mkdir(parents=True, exist_ok=True)
    with args.review_queue.open("w", encoding="utf-8", newline="") as review_file:
        review_file.write("game_id\tcurrent_title\tgamedb_title\tgdx_title\tpcsx2_title\t"
                          "get_config_title\treason\n")
        for review in sorted(reviews.values(), key=lambda item: item.game_id):
            values = (review.game_id, review.current_title, review.gamedb_title,
                      review.gdx_title, review.pcsx2_title, review.get_config_title, review.reason)
            review_file.write("\t".join(value.replace("\t", " ").replace("\n", " ")
                                        for value in values) + "\n")

    action_counts = Counter(change.action for change in changes)
    overlap_two = sum(1 for game_id in primary_ids
                      if sum(game_id in records for records in sources.values()) >= 2)
    overlap_three = sum(1 for game_id in primary_ids
                        if all(game_id in records for records in sources.values()))
    revisions = {
        "gamedb": git_revision(args.gamedb),
        "gdx": git_revision(args.gdx),
        "pcsx2": git_revision(args.pcsx2),
        "get_config": git_revision(args.get_config),
    }
    repository_root = args.output.parent
    changes_label = artifact_label(args.changes, repository_root)
    review_queue_label = artifact_label(args.review_queue, repository_root)
    report = f"""# V{release_label} database audit

Generated by `tools/audit_database.py`. The full machine-readable change and
review ledgers sit beside this report.

## Result

- Base records: **{len(base):,}**
- Final records and unique IDs: **{len(final):,}**
- Duplicate IDs: **0**
- Windows-unsafe titles: **0**
- Added primary-catalogue IDs: **{action_counts['added-primary-record']:,}**
- Removed unsupported prefix shadows: **{action_counts['removed-prefix-shadow']:,}**
- Automatic title corrections: **{sum(action_counts[name] for name in ('placeholder-replacement', 'abbreviation-expansion', 'three-source-correction')):,}**
- Human-reviewed title corrections: **{action_counts['reviewed-correction']:,}**
- Verified ID-changing English translations: **{len(translated):,}**
- Records queued for further manual review: **{len(reviews):,}**

## Reference snapshots

| Source | Parsed IDs | Git revision |
| --- | ---: | --- |
| GameDB-PS2 | {len(gamedb):,} | `{revisions['gamedb']}` |
| GDX English TitlesDB | {len(gdx):,} | `{revisions['gdx']}` |
| PCSX2 GameIndex (`name-en` preferred) | {len(pcsx2):,} | `{revisions['pcsx2']}` |
| Get_CONFIG support index | {len(get_config):,} | `{revisions['get_config']}` |

The primary-source union contains **{len(primary_ids):,}** IDs. **{overlap_two:,}**
occur in at least two primary catalogues and **{overlap_three:,}** occur in all
three. Get_CONFIG is used only as a guard/support source, not as a title voter.

## Conservative correction policy

1. Apply explicit reviewed-title and verified-translation manifests.
2. Add IDs missing from the base when a primary catalogue contains them.
3. Replace placeholders when catalogue evidence exists.
4. Expand an obviously incomplete title when at least two maintained catalogues
   agree and the existing normalized title is contained in the agreed title.
5. Replace any other differing title only when all three primary sources agree.
6. For a new ID without consensus, prefer PCSX2's English name, then GameDB,
   then GDX; retain ambiguous, richer, legacy, and custom titles and put them in
   the review queue instead of guessing.
7. Remove an unsupported `SCES`/`SLES`/`SCEE` prefix shadow only when the
   corresponding serial already exists in the base database.

## English-translation rule

`[English Translation]` is appended only when a documented unofficial patch
changes the executable game ID. Same-ID fan patches are deliberately excluded,
as are generic loader IDs that cannot identify one translation unambiguously.
The retail ID in each translation pair must remain present and unsuffixed.
V{release_label} currently contains one verified pair:

- `SCES_609.71` (patched; retail `SLPM_666.75`) — **Kingdom Hearts II - Final
  Mix + [English Translation]**

## Important reviewed corrections

- `SLES_511.68` — AFL Live 2003
- `SLES_518.26` — AFL Live 2004
- `SLES_519.03` — AFL Live 2004 - Aussie Rules Football
- `SLES_504.87` — Looney Tunes - Space Race
- `SLPM_620.89` — Wacky Races Starring Dastardly & Muttley [Cancelled Japanese Release]
- `SLPM_650.49` — Beatmania II DX 5th Style - New Songs Collection
- `SLPM_650.52` — Gitadora! GuitarFreaks 4th Mix & DrumMania 3rd Mix
- `SLKA_253.23` — FIFA 06
- `SLPM_602.65` — Steamboy [Trial]

## Known source conflicts

GDX contains **{len(gdx_conflicts)}** duplicate-ID conflicts. `SLUS_206.43`
maps to two unrelated titles there and is excluded from GDX voting; the
remaining conflicts are retained in the manual review ledger.

## Generated artifacts

- `gameid.txt` — canonical CRLF database embedded by the application
- `{changes_label}` — every add/remove/correction with reason
- `{review_queue_label}` — unresolved records and all source titles
- `tools/reviewed_titles.tsv` — repeatable human-reviewed decisions
- `tools/verified_translations.tsv` — narrowly scoped translation-ID evidence
"""
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(report, encoding="utf-8")

    return {
        "base": len(base), "final": len(final), "gamedb": len(gamedb),
        "gdx": len(gdx), "pcsx2": len(pcsx2), "get_config": len(get_config),
        "primary_union": len(primary_ids), "overlap_two": overlap_two,
        "overlap_three": overlap_three, "changes": len(changes), "reviews": len(reviews),
        **{f"action_{name}": count for name, count in action_counts.items()},
    }


def arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", type=Path, required=True)
    parser.add_argument("--gamedb", type=Path, required=True)
    parser.add_argument("--gdx", type=Path, required=True)
    parser.add_argument("--pcsx2", type=Path, required=True)
    parser.add_argument("--get-config", dest="get_config", type=Path, required=True)
    parser.add_argument("--reviewed", type=Path, required=True)
    parser.add_argument("--translations", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--changes", type=Path, required=True)
    parser.add_argument("--review-queue", dest="review_queue", type=Path, required=True)
    parser.add_argument("--release-version", default="4.1.0")
    return parser.parse_args()


def main() -> int:
    try:
        summary = audit(arguments())
    except (OSError, UnicodeError, ValueError) as error:
        print(f"audit failed: {error}", file=sys.stderr)
        return 1
    print(json.dumps(summary, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
