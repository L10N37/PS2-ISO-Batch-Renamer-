# V4.1 recovery checkpoint

Branch: `codex/v4.1-audit-online-db`
Base: `211fb88b5ff3b709a6fa8d52e270cdbb51d5c566`

## User constraints

- Keep all work on this local branch with frequent commits.
- Deliver a self-contained Fedora publishing script; do not push from Codex.
- GitHub Actions must build Windows and Linux, so no Windows boot is required.
- Use the online `gameid.txt` by default with a validated built-in fallback.
- Label an unofficial English translation only when its executable ID changes.
- Preserve unresolved database questions in a review queue instead of guessing.

## Recovered stage

- V4.1 version and online database core recovered.
- Strict 11-byte game-ID parsing recovered.
- libcurl/WinHTTP build integration and notices recovered.
- Remote `main` was verified unchanged at the required base before packaging.

## Next

Reconstruct the audited database, GUI integration, tests, documentation, release
workflow, and Fedora scripts; verify each stage and commit it locally.
