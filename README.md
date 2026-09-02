# PS2 Batch Renamer V4

A native desktop batch renamer for PlayStation 2 ISO and CHD images by
VajskiDs. V4 combines the V3 renamer and the former `GameName (game-id)` build
into one Dear ImGui application for Windows and Linux.

V4 is under active release preparation on the `v4-development` branch.

## What changed in V4

- One self-contained Windows executable; no .NET, console window, extraction,
  batch files, or helper executables.
- Native Linux application; Wine is not required.
- ISO and CHD files are read directly in-process. CHDs are not expanded into
  temporary ISO/CUE files.
- The current `gameid.txt` is compiled into the application. A newer or custom
  `gameid.txt` can be selected at runtime and the built-in copy restored with
  one click.
- Optional **Include game ID in filename** mode produces
  `Game Title (SLUS_123.45).iso` or `.chd`. With the option off, V3-style
  `Game Title.iso` / `.chd` names are used.
- Scan/preview and rename are separate steps. Nothing is overwritten.
- Every file appears in a live results table and activity view while work is
  running. Failed items produce a persistent red warning, a review prompt,
  and a one-click failed-items filter.
- `failed.txt` and `PS2_iso_files.txt` are no longer written into the games
  folder. Copying results or saving a report is optional.

## Using it

1. Download the Windows `.exe` or Linux `.AppImage` from
   [Releases](https://github.com/L10N37/PS2-ISO-Batch-Renamer-/releases).
2. Select or drag in the folder containing the games.
3. Select **ISO** or **CHD**, and optionally tick **Include game ID in
   filename**.
4. Choose **Scan / Preview**. Review the proposed names and any failures in
   the application window.
5. Choose **Apply Renames**.

The scan is non-recursive: only images directly inside the selected folder are
processed. A destination that already exists is reported as failed and is
never replaced.

On Linux, make the downloaded AppImage executable before opening it:

```bash
chmod +x PS2-Batch-Renamer-V4-Linux-x86_64.AppImage
```

## Database

The root [`gameid.txt`](gameid.txt) is the database embedded in a V4 build.
Select **Choose newer gameid.txt...** in the app to use an external update for
the current session. V4 preserves database order and the established
first-match-wins behavior for duplicate IDs.

Each non-empty database line must contain an 11-character game ID, one space,
then the title:

```text
SLUS_217.20 Arcana Heart
```

The historical additions and corrections are in
[`docs/database-changes.md`](docs/database-changes.md).

## Identification method

The proven handwritten identification method from the earlier releases is
preserved: V4 reads the root-directory sector stored at `0x80A4`, searches the
2,048-byte root directory for `EM.CNF`, follows the original extent bytes to
`SYSTEM.CNF`, finds `0:\`, and reads the following 11-byte game ID. The same
logic is used for ISO and for the cooked disc data exposed by a CHD.

See [`docs/algorithm.md`](docs/algorithm.md) for the parity notes and regression
test coverage.

## Building

V4 uses CMake 3.24+, C++20, Dear ImGui, SDL3, and libchdr. Dependencies are
pinned and fetched by CMake.

```bash
cmake -S . -B build -DPS2BR_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Linux builds need the normal X11/OpenGL development packages. The GitHub
Actions workflow is the reference build environment for both release targets.

## Legacy versions

The earlier Windows-only tools and their original source are retained under
[`legacy/`](legacy/) for project history. V4 does not execute or bundle those
programs.

## License

PS2 Batch Renamer is distributed under the
[GNU General Public License v3.0](LICENSE.txt). Dependency notices are embedded
in the app's **About** window and are also available in
[`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md).
