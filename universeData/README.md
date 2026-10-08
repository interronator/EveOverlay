# EVE universe map data

Compiled from [DOTLAN EVE Maps](https://evemaps.dotlan.net/) region pages (see [docs/CREDITS.md](../docs/CREDITS.md)).

- `systems.csv`: header row, then Name,Region,Security,SovHolder,Stats,Gates (Gates = neighbours separated by `;`, empty if none). 5528 systems. The program reads this file at runtime.
- `gates.txt`: one line per system: `Name [Region Sec] -> neighbours` (undirected, deduped across region pages). Reference only; the program does not load it.

Known gaps: ~2% of drawn links on the source pages could not be tied to a node (mostly cross-region stubs); most are recovered from the neighbouring region's page. Systems with no links are mainly wormhole/Jove-space pages (A821-A, J7HZ-F, UUA-F4). Sov holder is a snapshot from when the data was compiled.
