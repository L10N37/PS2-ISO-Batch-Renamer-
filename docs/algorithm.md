# V4 algorithm parity notes

V4 deliberately preserves the successful identifier and database lookup
behavior of V3 and the `GameName (game-id)` variant. The desktop interface,
error handling, and CHD input path are new; the identification sequence and
generated names are not redesigned.

## Disc identification sequence

1. Read two bytes at disc offset `0x80A4` as a big-endian root-directory
   sector number.
2. Multiply that value by the 2,048-byte disc sector size.
3. Read the 2,048-byte root directory and locate the first `EM.CNF` byte
   sequence.
4. Move back 31 bytes from that match and read the four-byte big-endian
   `SYSTEM.CNF` extent.
5. Multiply the extent by 2,048 and read the first 64 bytes of `SYSTEM.CNF`.
6. Locate the first byte sequence `30 3A 5C` (`0:\`).
7. Read the following 11 bytes as the game ID.
8. Search the database from top to bottom and use the first line whose first
   11 characters match the extracted ID.

For CHD input, libchdr exposes the cooked 2,048-byte data sectors directly to
the same identifier. V4 does not extract the complete image or invoke
`chdman.exe`, `ps2_chd.exe`, or `chd_ps2_renamer.exe`.

## Filename parity

| Option | Result |
|---|---|
| Include game ID off | `Database Title.iso` or `Database Title.chd` |
| Include game ID on | `Database Title (GAME_ID).iso` or `.chd` |

V4 does not silently rewrite a database title. A title containing a character
that Windows cannot use in a filename is shown as a failed preview item on
both platforms. This keeps Windows and Linux outcomes consistent while leaving
the database and naming rule intact.

## Regression coverage

The automated core tests verify:

- the original root-directory and `SYSTEM.CNF` offsets;
- exact 11-byte game-ID extraction;
- first-match behavior for duplicate database IDs;
- both title-only and title-plus-ID naming;
- malformed images and missing IDs remaining visible as failures;
- destinations never being overwritten;
- no automatic `failed.txt` or `PS2_iso_files.txt` output;
- an ISO-to-CHD round trip when `chdman` is available in the test environment.
