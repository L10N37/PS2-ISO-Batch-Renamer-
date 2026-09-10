# V4.1 release checkpoint

Branch: `codex/v4.1-audit-online-db`
Base: `211fb88b5ff3b709a6fa8d52e270cdbb51d5c566`

## User constraints

- Keep all work on this local branch with frequent commits.
- Deliver a self-contained Fedora publishing script; do not push from Codex.
- GitHub Actions must build Windows and Linux, so no Windows boot is required.
- Use the online `gameid.txt` by default with a validated built-in fallback.
- Label an unofficial English translation only when its executable ID changes.
- Preserve unresolved database questions in a review queue instead of guessing.

## Completed work

- V4.1 version, strict database parser, and online database core are complete.
- Linux uses libcurl and Windows uses WinHTTP, with a complete embedded fallback.
- Online refresh runs independently of scanning and only installs a complete,
  validated database while no scan or rename is active.
- The four-catalogue audit is reproducible from the checked-in manifests and
  preserves unresolved disagreements for manual review.
- The generated database contains 13,442 unique canonical IDs, including 2,041
  primary-source additions and 2,339 removed unsupported prefix shadows.
- The full 6,170-change ledger and 4,087-entry review queue are checked in.
- The AFL year, Looney Tunes, Wacky Races, Beatmania/Gitadora, FIFA 06, and
  Steamboy mappings are locked by regression tests.
- `SCES_609.71` is the only verified ID-changing English translation; retail
  `SLPM_666.75` remains present and unsuffixed.
- Fedora database/release scripts and a tag-triggered Windows/Linux GitHub
  Actions release workflow are included with complete V4.1.0 release notes.
- Remote `main` was verified unchanged at the required base before packaging.

## Verification

- Both online/libcurl and explicit offline/no-network CMake builds pass all core
  regression tests.
- The GUI translation units pass warning-enabled syntax compilation against the
  exact SDL3 and Dear ImGui revisions pinned in CMake.
- Regenerating the database from the completed database produces a byte-identical
  `gameid.txt`.
- The database contains 13,442 rows, 13,442 unique IDs, CRLF throughout, and one
  verified translation suffix.
- Workflow YAML, Bash syntax, Python compilation, argument guards, and
  `git diff --check` all pass.

## Handoff

Package this branch into one self-contained Fedora publisher. The publisher must
verify remote `main` is still the exact base, build/test before pushing, require
an explicit confirmation, push `main` and annotated `v4.1.0`, then let GitHub
Actions create the noted Windows/Linux release. Codex itself must not push.
