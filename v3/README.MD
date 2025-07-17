
# PS2 Batch Renamer GUI

A simple WPF app to batch rename PS2 game ISOs and CHDs.

---

## Features

- Browse and select a folder containing your PS2 games.
- Copies a bundled ZIP with required tools to the selected folder.
- Extracts the ZIP contents directly inside the selected folder.
- Runs a batch script to rename games (`ps2.exe` for ISO, `chd_ps2_renamer.exe` for CHD).
- Cleans up extracted tools and temporary files after the renaming completes.

---

## How It Works

1. **Select Folder:**  
   Use the folder browser dialog to pick your games directory.

2. **Copy & Extract:**  
   The app copies `PS2_batch_renamer.zip` from its own folder into the selected directory, then extracts all contents there.

3. **Run Renamer:**  
   Depending on your choice (ISO or CHD), it launches `run_renamer.bat` with the appropriate parameter to run the correct renamer executable.

4. **Cleanup:**  
   After the renaming process finishes, the app deletes all extracted executables and helper files, leaving your folder clean.

---

## Usage

- Choose ISO or CHD mode via radio buttons.
- Click "Select Folder" and pick your game folder.
- Click "Run Renamer" to start the process.
- Follow on-screen logs for status and completion messages.

---

## Notes

- The batch script handles the actual running of renamer EXEs and deletes them afterward.
- The app deletes the ZIP file immediately after extraction.
- The console window remains open during renaming so you can see progress and errors.
