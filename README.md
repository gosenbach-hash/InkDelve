# InkDelve

A tile-reveal dungeon crawler for the PocketBook Era Color e-reader (7" Kaleido 3 color e-ink, firmware 6.x).
Tap tiles to reveal them, fight monsters, collect gold and potions, find the stairs, and shop every 5 floors.
Your run is saved after every move, and the best score is kept.

## Layout
- `core/`: portable C++ game logic (grid, combat, scoring, save/load) with unit tests. No UI code.
- `ui/`: thin PocketBook InkView app that draws the board and handles taps.

## Build
Needs Docker and the PocketBook SDK **B300-6.8** (not included; download `SDK-B300-6.8.7z` from the
[PocketBook SDK releases](https://github.com/pocketbook/SDK_6.3.0/releases) and extract it into a Docker
volume named `inkdelve-sdk`, so it ends up at `/BUILD/SDK-B300-6.8` inside the build container).

Run the core tests:

```
MSYS_NO_PATHCONV=1 docker run --rm -v "<repo>/core:/src:ro" ubuntu:22.04 sh /src/test.sh
```

Build `ui/inkdelve.app`:

```
MSYS_NO_PATHCONV=1 docker run --rm -v inkdelve-sdk:/BUILD -v "<repo>:/src" ubuntu:22.04 sh /src/ui/build.sh
```

## Install
Connect the reader by USB and copy `inkdelve.app` into its `applications` folder. It then appears in the Applications menu.

## Notes
Game assets are drawn in code; no third-party art is used.
