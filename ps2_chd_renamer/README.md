
# PS2 CHD Batch Renamer

This utility batch-processes your **PlayStation 2 CHD files**, converting them to ISO format using `chdman.exe`, extracting their game ID using `ps2_chd.exe`, renaming the **original CHD files**, and **deleting** the temporary ISO and CUE files after processing.

---

## ⚙️ Requirements

Place **all of the following files** in the **same folder** as your PS2 `.chd` games:

- `chdman.exe`  
  📥 Download it from: [Recalbox chdman Utility](https://wiki.recalbox.com/en/tutorials/utilities/rom-conversion/chdman)
- `ps2_chd.exe`  
  The utility that reads `SYSTEM.CNF` from the converted ISOs to extract the Game ID and rename.
- `gameid.txt`  
  A lookup table mapping PS2 Game IDs to their full game titles.
- `chd_ps2_renamer.exe`  
  The **main batch renaming executable** that handles conversion, renaming, and cleanup.

---

## 🚀 How to Run

Simply run the following file:

```bash
chd_ps2_renamer.exe
```

It will:

1. Scan the folder for `.chd` files.
2. Convert **4 CHDs at a time** to ISO using `chdman`.
3. Extract the PS2 Game ID using `ps2_chd.exe`.
4. Rename the **original CHD** file based on the matched game title.
5. Delete the temporary `.iso` and `.cue` files.
6. Repeat until all CHD files are processed.

---

## ⚠️ Storage Warning

Each batch of 4 CHD files is temporarily extracted to ISO format.

You must ensure **at least ~32 GB** of free space on the drive where you're running this from, as each PS2 ISO can be **up to 7.95 GB**.

---

## 📂 Example Folder Layout

```
your-folder/
├── chdman.exe
├── ps2_chd.exe
├── chd_ps2_renamer.exe
├── gameid.txt
├── SLUS_210.05.GT4.chd
├── SLES_524.58.Burnout3.chd
├── ...
```

---

## 📝 Notes

- `ps2_chd.exe` only renames CHD files **after a successful conversion** and **lookup**.
- ISO and CUE files are deleted **only after** successful renaming.
- Failed lookups (e.g., no Game ID match in `gameid.txt`) will be logged in `failed.txt`.

---

Happy dumping and renaming! 🎮✨
