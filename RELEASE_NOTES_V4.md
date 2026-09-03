# PS2 Batch Renamer V4

V4 replaces the earlier Windows-only variants with one clean native Dear ImGui
application for Windows and Linux.

## Highlights

- One self-contained Windows EXE with an embedded application icon.
- Native Linux AppImage; Wine is not required.
- ISO and CHD images are identified directly in-process.
- Choose title-only names or tick **Include game ID in filename** for
  `Game Title (GAME_ID)`.
- The current 13,791-record `gameid.txt` is built in, with an option to load
  a newer or custom database for the current session.
- Legacy database `^!` escaping and trailing padding are removed from
  generated filenames.
- Includes 36 documented unofficial Usagiru `USGR` compilation IDs.
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

The proven handwritten V3 identification sequence and first-database-match
behavior are preserved. V4 changes the interface, packaging, CHD input path,
and error reporting—not the established disc-identification algorithm.

The earlier programs and their source remain archived under `legacy/`.
