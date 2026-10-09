# Eve Overlay

A free Windows tool for EVE Online players who run several game clients at once.

- **Live previews** of every EVE client in small floating windows
- **Click a preview, or use a global hotkey** (set in the settings window) to jump to that client
- **Intel Watcher map** that flashes and plays a sound when your intel channel reports a nearby system, with a list of recent reports, priority keywords, quieter alerts for distant systems, and optional jump bridges
- **Attack alerts** that make a client's preview blink red when that character is shot at or warp scrambled
- **Hotkeys** for each client, for stepping through groups of clients, and for hiding all previews or minimizing all clients
- **Timers and d-scan tools**: countdown timers, a scan-age counter, and a summary of any d-scan you copy from the game
- Lives in the system tray, one small `.exe`, nothing to install

*Not affiliated with or endorsed by CCP Games. EVE Online is a trademark of CCP hf.*

## Download and run (no building needed)

1. Go to the [Releases page](../../releases/latest) and download **EveOverlay-x.y.z-win64.zip**.
2. Right-click the zip, choose **Extract All...**, and pick a folder like your Desktop or Documents. Avoid `C:\Program Files`, because the program saves its settings next to itself.
3. Open the extracted folder and double-click **EveOverlay.exe**.
4. Start your EVE clients. Previews appear on their own. Find the settings window from the Eve Overlay icon in the system tray (bottom right, near the clock).

The first time you run it, Windows may show a blue **"Windows protected your PC"** box because the program is not code-signed. Click **More info**, then **Run anyway**.

To remove it, delete the folder.

## Using the Intel Watcher

1. In EVE, open the intel chat channel you want to watch.
2. In Eve Overlay, open the **Universe** tab, choose your home system and how many jumps to watch, and type the channel name.
3. Turn on **Show map**. Reported systems flash on the map, and you can enable a sound alert.

## More tools

- **Attack Alerts tab.** Reads the game logs EVE writes to `Documents\EVE\logs\Gamelogs`. A preview blinks red and a sound plays when its character takes damage or is warp scrambled. The client you are playing is never flashed.
- **Hotkeys tab.** Click a box and press keys. Give each character a hotkey, make cycle groups (next and previous client, in the order the previews sit on screen), and set keys to hide all previews or minimize all clients.
- **Timers tab.** Start countdowns, set a quick-timer hotkey, and press a hotkey after each d-scan to see how old your scan is. A small panel floats over the game; hold Alt and drag it to move it.
- **D-Scan tab.** Turn on automatic reading, then copy the results of the in-game directional scanner. A summary of what is on scan appears, with Black Ops, interdictors, recons, capitals and other ships worth a look flagged. Nothing is read from the clipboard until you switch this on.
- **Intel Watcher extras.** Skip systems reported clear (`clr`, `nv`) and questions, follow your own system from the Local chat log, watch several channels at once (separate names with commas), and set priority keywords with their own sound. To count jump bridges as one jump, open `Jump Bridges.txt` from the Intel Watcher tab and add one per line, like `Jita » Perimeter`.
- **Updates.** The About tab can check GitHub for a newer version when you press the button; it never checks on its own.

Nothing here sends keys or clicks to the game, and none of it needs you to log in anywhere.

## Requirements

Windows 10 or 11 (64-bit). Nothing else.

## Building from source

Only needed if you want to change the code. You need Visual Studio 2022, CMake and vcpkg; the short version is to double-click `build.bat`. The exe should be in `Binaries/Release`. Details are in [docs/BUILDING.md](docs/BUILDING.md).

To make the release zip yourself, build first, then run `powershell -ExecutionPolicy Bypass -File package.ps1`. Pushing a tag like `v1.0.0` makes GitHub build and publish the zip automatically (see `.github/workflows/release.yml`).

## What is in each folder

| Folder / file | What it is |
|---|---|
| `src/` | The program's source code ([overview](src/README.md)) |
| `assets/` | Icon, Windows resource files and the default alert sound |
| `universeData/` | EVE universe map data (systems and stargates) for the Intel Watcher, and the ship list for the d-scan reader |
| `tools/` | A script that rebuilds the ship list from CCP's static data |
| `tests/` | Automated checks and an end-to-end test script |
| `docs/` | Build instructions and credits |
| `build.bat`, `package.ps1` | One-click build, and release zip packaging |
| `CMakeLists.txt`, `CMakePresets.json`, `vcpkg.json` | Build configuration |

## Images

<img width="878" height="1228" alt="image" src="https://github.com/user-attachments/assets/fbb82cfb-3c0f-48df-b472-732f9f2c3b11" />

<img width="882" height="1230" alt="image" src="https://github.com/user-attachments/assets/545196d2-5c2a-4f5e-b1e7-eb67b38e99bb" />

<img width="370" height="257" alt="image" src="https://github.com/user-attachments/assets/f25b9ed3-4b8d-4e08-90ba-571c5c406ae7" />

<img width="877" height="1228" alt="image" src="https://github.com/user-attachments/assets/c1b6f293-8d82-4e71-8a24-d63954c29795" />

## License and credits

Released under the [MIT License](LICENSE). Third-party libraries and data sources are listed in [docs/CREDITS.md](docs/CREDITS.md). Eve Overlay is a C++ rewrite inspired by the original EVE-O Preview.
