# assets

Non-code files that get built into or shipped next to the program.

- `icon.ico`, `EveOverlay.rc`, `app.manifest` — app icon, version info and Windows manifest.
- `sounds/` — alert sounds, copied next to the exe on build. Ships with one generated default (`defaultWarning.wav`).

To use your own alert sound, drop an `.mp3`, `.wav` or `.wma` file into the `sounds` folder next to `EveOverlay.exe`, or pick any file from the Universe tab.
