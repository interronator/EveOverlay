# tests

- `*Probe.cpp` — small programs that check one area each (settings, services, universe map, UI...). Each prints `ALL PASSED` or `OK` on success. Build them with `build.bat tests`.
- `qa/` — an end-to-end script (`Run-Qa.ps1`) that drives the real app with a fake EVE client (`FakeClient.cpp`). See [docs/BUILDING.md](../docs/BUILDING.md) before running it.
