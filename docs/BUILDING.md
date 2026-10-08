# Building and testing

## Requirements
- Visual Studio 2022 (C++ desktop workload, which includes ATL), CMake 3.21+, git
- [vcpkg](https://github.com/microsoft/vcpkg) with `VCPKG_ROOT` set (manifest mode pulls `nlohmann-json`, `wtl` and `imgui` with the DX11 and Win32 bindings)

## Build
```
cmake --preset default
cmake --build --preset release
```
Output is in `Binaries\Release\` (or `Binaries\Debug\`): the exe plus `universeData\systems.csv`, which the Universe Map reads at runtime, and a `sounds` folder with the default alert sound. `build.bat` wraps both steps. `EveOverlay.exe` links the C++ runtime statically and depends only on Windows system DLLs.
`build\qa\Release\ExeFile.exe` is a fake EVE client for the QA script, not part of the product.

## Tests
| What | How |
|---|---|
| Config, services, signals, single-instance guard, tray, Active Clients tab | run `Binaries\Release\ConfigProbe.exe <dir>`, `CoreProbe.exe`, `ServicesProbe.exe`, `UiProbe.exe` (each prints `ALL PASSED`; `ConfigProbe` needs a writable directory argument) |
| End to end against the real app | `powershell -File tests\qa\Run-Qa.ps1 [-Section input\|snap\|hide\|resize\|minimize\|layout\|rename\|leak]` |

`Run-Qa.ps1` drives the app with real mouse and keyboard input on an interactive desktop, so leave the mouse alone while it runs
(a section takes a few seconds to a minute). It overwrites and deletes `Eve Overlay.json` next to the built exe and stops any
`ExeFile.exe`, so do not run it with real EVE clients open.

## Not covered by the automated tests
- Mixed-DPI monitors and live DPI changes (all test monitors were 96 DPI)
- Real EVE clients (the QA script uses a fake window)
- The unhandled-exception handler (it shows a message box)
