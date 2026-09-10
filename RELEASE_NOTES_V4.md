# PS2 Batch Renamer V4.1.0

V4.1.0 adds a validated online-first database with an embedded offline fallback
and replaces the historical title list with a reproducible, conservative audit.
The original V3 disc-identification algorithm remains unchanged.

## Online database with safe fallback

- The app checks the repository's current `main/gameid.txt` automatically at
  startup without blocking folder selection or scanning.
- A download is accepted only after the entire database parses successfully,
  contains at least 10,000 unique records, and remains below the 4 MiB limit.
- HTTP errors, timeouts, duplicate IDs, malformed records, or a partial file
  leave the embedded database active and produce a visible status message.
- **Refresh online**, **Choose local gameid.txt...**, and **Reset to built-in**
  controls make the active source explicit.
- No downloaded database is written to the user's machine.

## V4.1 database audit

The historical database was compared against pinned snapshots of GameDB-PS2,
the GDX English TitlesDB, the PCSX2 GameIndex, and the Get_CONFIG PS2 index. The
generated database contains:

- **13,442 records**
- **13,442 unique canonical game IDs**
- **0 duplicate IDs**
- **0 Windows-unsafe titles**
- **2,041 missing primary-catalogue IDs added**
- **2,339 unsupported prefix-shadow records removed**
- **1,780 conservative automatic title corrections**
- **4,087 unresolved entries retained in a manual review queue**

Important reviewed corrections include:

- `SLES_511.68` — **AFL Live 2003**
- `SLES_518.26` — **AFL Live 2004**
- `SLES_519.03` — **AFL Live 2004 - Aussie Rules Football**
- `SLES_504.87` — **Looney Tunes - Space Race**
- `SLPM_620.89` — **Wacky Races Starring Dastardly & Muttley
  [Cancelled Japanese Release]**
- `SLPM_650.49` — **Beatmania II DX 5th Style - New Songs Collection**
- `SLPM_650.52` — **Gitadora! GuitarFreaks 4th Mix & DrumMania 3rd Mix**
- `SLKA_253.23` — **FIFA 06**
- `SLPM_602.65` — **Steamboy [Trial]**

The complete methodology and source revisions are in
[`docs/database-audit-v4.1.0.md`](docs/database-audit-v4.1.0.md). Every change
is in [`docs/database-changes-v4.1.0.tsv`](docs/database-changes-v4.1.0.tsv),
and unresolved catalogue disagreements are retained in
[`docs/database-review-v4.1.0.tsv`](docs/database-review-v4.1.0.tsv).

## Unofficial English translations

`[English Translation]` is appended only when evidence shows that a fan patch
changes the executable game ID. Same-ID patches and generic loader IDs are not
labelled because the ID alone cannot identify a particular translation.

V4.1.0 includes the verified patched ID `SCES_609.71` as **Kingdom Hearts II -
Final Mix + [English Translation]**. Its retail ID `SLPM_666.75` remains a
separate, unsuffixed record.

## Reliability and portability

- Strict database parsing now rejects malformed IDs with the physical line
  number while retaining legitimate alphanumeric product IDs such as
  `KOEI_SP0.02`.
- Linux uses libcurl for HTTPS; Windows uses the built-in WinHTTP API.
- Third-party source snapshots are pinned by both commit and SHA-256; release
  builds retry cleanly after an interrupted or failed dependency download.
- The online worker is independent of the scan/rename worker. A validated
  update waits until active file work finishes before replacing the in-memory
  database.
- Regression tests cover the reviewed titles, translation/retail pair, strict
  ID parsing, duplicate and incomplete downloads, oversize protection, and the
  offline fallback build.

## Downloads

- **Windows:** `PS2-Batch-Renamer-V4.exe`
- **Linux:** `PS2-Batch-Renamer-V4-Linux-x86_64.AppImage`
- **Standalone/legacy database:** `gameid.txt`
- **Integrity:** `SHA256SUMS.txt`

On Linux:

```bash
chmod +x PS2-Batch-Renamer-V4-Linux-x86_64.AppImage
./PS2-Batch-Renamer-V4-Linux-x86_64.AppImage
```

The selected game folder must be writable. A dual-boot NTFS volume left
hibernated by Windows or mounted read-only by `fuseblk` cannot be renamed until
it is cleanly remounted read/write.

## Preserved behavior

The proven handwritten V3 identification sequence and local external-database
first-match behavior are preserved. V4.1.0 changes database selection,
validation, catalogue quality, and release tooling—not the disc-identification
algorithm.

The earlier programs and their source remain archived under `legacy/`.
