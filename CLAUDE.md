# InkDelve: a tile-reveal dungeon crawler for PocketBook Era Color

Personal project, no employer info. Full details in BRIEF.md (read only when needed).

## Rules
- Ask for a go-ahead before anything token-heavy (agents, workflows, broad searches, big reads) or anything not directly requested.
- Ask before downloading any file (state name, source, size).

## Decisions
- Language: C++. v1 scope: bare loop (milestones 1-3).
- Own names/art/tuning; never reuse assets from other games.

## Target
- PocketBook Era Color (700K3), 7" Kaleido 3 color e-ink, firmware U700K3.6.10.2359.
- Flat bold colors, no gradients, no animation, minimal refreshes.
- SDK: github.com/pocketbook/SDK_6.3.0 (per-firmware branches; 6.8 newest known). Era Color SoC unknown.
- Build: Linux x86_64 toolchain via WSL2/Docker, CMake, output `.app`; sideload to the reader's `applications` folder.

## Architecture
1. `core/`: portable C++ game logic, no UI, seeded level gen, save/load; unit-tested natively on Windows.
2. `ui/`: thin InkView app (init/show/key/touch/exit handler via `InkViewMain`).
3. Art as compiled-in glyphs/bitmaps.
4. Save after every move and on exit.

## Milestones
1. Hello-world `.app` on device. 2. Grid reveal + adjacency. 3. Combat, items, stairs. 4. (later) shops, chests, classes.

## Build (verified)
SDK-B300-6.8 is extracted in Docker volume `inkdelve-sdk` (archive in sdk/). Build the UI from Git Bash:
`MSYS_NO_PATHCONV=1 docker run --rm -v inkdelve-sdk:/BUILD -v "C:\Games\InkDelve:/src" ubuntu:22.04 sh /src/ui/build.sh`
Output: ui/inkdelve.app. Toolchain is clang 7 (armv7, softfp), needs libtinfo5 in the container.

## Status
Milestone 1 and 2 done and verified on device: tappable 5x4 tile grid (ui/main.cpp) over core/grid, partial refresh on reveal, New game button, portrait locked, status panel off. Core tests: core/test.sh in Docker. Milestone 3 core done (core/game.{h,cpp}: hero HP/gold, combat, pickups, stairs; tests in core/test_game.cpp). UI wired to Game (HUD, monster HP on tiles, message line, game over via message), verified on device. Gold/potions/monsters: tap reveals, second tap collects/fights. Milestone 3 done. Save/load: Game::serialize/load in core (tested); UI saves to STATEPATH/inkdelve.sav after every move and on exit, resumes on launch, deletes on death (verified on device). Graphics: flat icons drawn in code (ui/gfx.{h,cpp}: skull, coin, potion, stairs, HUD heart/coin/stairs), verified on device. Monster rules: monsters hit first each round; a revealed monster locks all 8 surrounding hidden tiles (drawn as X, diagonals included) until killed; movement is still orthogonal; level gen never puts monsters orthogonally next to each other (diagonal is fine). Verified on device. Tuning pass (verified on device): grid 6x5 (Game::kCols/kRows; old 5x4 saves are rejected), entrance on an edge, stairs >=4 Manhattan from entrance, 20% gold / 12% potions / 40% monsters, board-clear bonus 10+5*depth gold (TapResult::Cleared). Shop (verified on device): after finishing every 5th level a shop screen offers +1 attack and heal-to-full +5 max HP at 100 gold each (repeatable) plus Continue; save format INKDELVE2 adds the shop flag (v1 still loads). Scoring (built, not device-tested): score = 100*floor + 10*kills + total gold acquired (Game::score; save format INKDELVE3 adds kills and totalGold, v1/v2 still load); HighScore (score + floor) saved to STATEPATH/inkdelve.hi on death or when abandoning a run via New game; score line shown under the New game button. Graphics glow-up (built, not device-tested): gfx::drawTile draws raised hidden tiles (brighter when tappable), outlined revealed tiles, horned/glowing-eye skulls by depth, coin piles, shaded potion, stone stairs, floor pebbles, hero on the entrance tile (Grid::entrance, recovered on load), HP bar HUD, game-over panel, shop icons. Home-screen icon: icons/ has inkdelve.bmp + inkdelve_f.bmp + view.json attempt; on the Era Color (6.10) the object-path view.json entry made the app vanish from the menu (even the generic icon), so the user restored the original view.json. Not working; the wiki absolute-path form was not tried. Next: chests, keys, classes, balance.
