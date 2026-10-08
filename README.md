# Eve Overlay

A free Windows tool for EVE Online players who run several game clients at once.

- **Live previews** of every EVE client in small floating windows
- **Click a preview, or use a global hotkey** (set in the settings window) to jump to that client
- **Intel Watcher map** that flashes and plays a sound when your intel channel reports a nearby system
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
| `universeData/` | EVE universe map data (systems and stargates) used by the Intel Watcher |
| `tests/` | Automated checks and an end-to-end test script |
| `docs/` | Build instructions and credits |
| `build.bat`, `package.ps1` | One-click build, and release zip packaging |
| `CMakeLists.txt`, `CMakePresets.json`, `vcpkg.json` | Build configuration |

## License and credits

Released under the [MIT License](LICENSE). Third-party libraries and data sources are listed in [docs/CREDITS.md](docs/CREDITS.md). Eve Overlay is a C++ rewrite inspired by the original EVE-O Preview.
