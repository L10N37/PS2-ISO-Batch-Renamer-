# PS2 Batch Renamer V4.0.1

V4.0.1 is a database-quality update to the native Dear ImGui release. The
identification algorithm and application workflow are unchanged.

## Database corrections

- Audited all 51 duplicate IDs inherited from the historical database.
- Removed 16 exact duplicate records.
- Resolved 35 alternate or conflicting title records against current PS2
  serial/title references.
- Corrected notable mismatches including:
  - `SLES_520.05` to **007 - Everything or Nothing**
  - `SLES_529.74` to **GoldenEye - Rogue Agent**
  - `SLES_533.57` to **21 Card Games**
  - `SLES_533.66` to **Killer7**
  - `SLES_549.98` to **Mercenaries 2 - L'enfer des Favelas**
- Corrected 17 additional titles containing Windows-forbidden filename
  characters.
- The built-in database now contains **13,740 records, 13,740 unique IDs, and
  zero duplicate IDs**.
- The 36 documented unofficial Usagiru `USGR` compilation IDs remain included
  and are all unique.

The full audit is documented in
[`docs/database-audit-v4.0.1.md`](docs/database-audit-v4.0.1.md).

## V4 highlights

- One self-contained Windows EXE with an embedded application icon.
- Native Linux AppImage; Wine is not required.
- ISO and CHD images are identified directly in-process.
- Choose title-only names or tick **Include game ID in filename** for
  `Game Title (GAME_ID)`.
- The audited `gameid.txt` is built in, with an option to load a newer or
  custom database for the current session.
- Legacy database `^!` escaping and trailing padding are removed from
  generated filenames.
- Live per-file scanning and renaming activity.
- Persistent in-app failure warning and one-click failed-items review.
- Scan/preview is separate from applying renames.
- Existing destination files are never overwritten.
- No automatic `failed.txt`, file lists, helper programs, or temporary
  extracted images are placed in the games folder.

## Downloads

- **Windows:** `PS2-Batch-Renamer-V4.exe`
- **Linux:** `PS2-Batch-Renamer-V4-Linux-x86_64.AppImage`
- **Integrity:** `SHA256SUMS.txt`

On Linux:

```bash
chmod +x PS2-Batch-Renamer-V4-Linux-x86_64.AppImage
./PS2-Batch-Renamer-V4-Linux-x86_64.AppImage
```

The selected games folder must be writable. Dual-boot Windows NTFS volumes left
hibernated or mounted read-only cannot be renamed until remounted read/write.

## Preserved behavior

The proven handwritten V3 identification sequence and external-database
first-match behavior are preserved. V4.0.1 changes the bundled database and its
validation—not the established disc-identification algorithm.

The earlier programs and their source remain archived under `legacy/`.
