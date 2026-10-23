# InkDelve for PocketBook Era Color: project brief

Personal project. Contains no employer information. Safe to copy to a new folder (e.g. `C:\Games\InkDelve`).

## Goal
A simple tile-reveal dungeon crawler (played like minesweeper) that runs natively on a PocketBook Era Color e-reader, in the style of a 2012/13 mobile game. Own names, art and tuning; reuse no third-party assets.

## Target device
- PocketBook Era Color (model 700K3): 7" Kaleido 3 color e-ink, touch, Linux-based firmware.
- Firmware: U700K3.6.10.2359 (6.10 line).
- Color layer is ~150 ppi, so use a few flat, bold colors and simple icons; no gradients.

## SDK findings (checked 2026-10-19)
- Official SDK: `github.com/pocketbook/SDK_6.3.0`. Despite the name, each firmware generation lives in its own branch. Pick the branch matching the firmware.
- MobileRead (June 2024) reports an SDK 6.8 release in that repo, as two archives, one per SoC (a forum guess says Allwinner B288 dual-core and B300 quad-core; unconfirmed).
- No source found for a 6.10 SDK. 6.8 is the newest documented. Building against an older SDK usually runs on newer firmware, but verify with a hello-world on the device.
- Unknown: which SoC the Era Color uses (decides which 6.8 archive).
- Community Rust bindings exist (`simmsb/inkview-rs`), unofficial. C/C++ via InkView is the supported route.
- Sources: github.com/pocketbook, codeberg.org/plyr0/SDK_6.3.0, mobileread.com/forums/showthread.php?p=4433294, github.com/simmsb/inkview-rs.

## Build and deploy
- Toolchain is Linux x86_64: use WSL2 (or Docker) on Windows.
- Cross-compile to a `.app` binary with CMake and the SDK toolchain.
- Connect reader by USB, copy the `.app` to the `applications` folder. It then appears in the Applications menu. No signing needed for sideloading.
- App skeleton: `main()` calls `InkViewMain(handler)`; handler receives init/show/key/touch/exit events. Draw with InkView calls (`DrawTextRect`, `FillArea`, bitmaps); refresh with `PartialUpdate`/`SoftUpdate`/`FullUpdate`. Plan for e-ink: few updates, no animation, occasional full refresh to clear ghosting. Verify exact call names in the SDK headers.

## Game design (adjust freely)
- Small grid of face-down tiles (roughly 5x3 to 6x4). Only tiles adjacent to revealed ones can be revealed.
- Tile contents: monsters (HP, attack), gold, potions, weapons/armor, chests, keys, traps, shops, altars, exit stairs.
- Tapping an enemy trades blows; surviving enemies hit back as you act.
- Goal: descend as deep as possible. Hero classes with abilities add replayability.
- Tap-driven and turn-based, so no animation is needed; only the tapped tile region needs a partial refresh.

## Architecture
1. Game core: portable C++, no UI code. Grid, combat, loot tables, level generation (deterministic from a seed), save/load. Unit-test natively on Windows.
2. UI layer: thin InkView app that draws tiles, handles taps, calls the core. Only part needing the Linux toolchain and device.
3. Art: simple glyph-style icons (bitmaps compiled in, or font glyphs).
4. Persistence: save state after every move and on exit so the game resumes.

## Milestones
1. Hello-world `.app` running on the Era Color (proves toolchain).
2. Tile grid with reveal and adjacency rules, no combat.
3. Combat, items, stairs.
4. Shops, chests, classes and abilities, then balance.

## Open questions
- Language: C++ (recommended) vs something else.
- Scope of v1: bare loop (milestones 1-3) or full feature set.
- Whether the SDK emulator shows color usefully.
- Era Color SoC and the correct SDK 6.8 archive.

## Cost estimate (API-equivalent, rough; prices cached 2026-10-06)
Full build ~150 turns: ~16.5M cache-read, ~0.6M cache-write, ~0.4M output tokens.

| Model | $/M in / out | Full game | Bare loop |
|---|---|---|---|
| Haiku 5.5 | 0.10 / 0.50 | ~$0.45 | ~$0.20 |
| Sonnet 5.5 | 2 / 10 | ~$9 | ~$4 |
| Opus 5.5 | 4 / 20 | ~$14 | ~$6.50 |
| Fable 5.1 | 10 / 50 | ~$32 | ~$14 |

Treat as 0.5x to 2x. Recommendation: Sonnet 5.5 by default, Opus 5.5 for hard InkView/toolchain debugging. A fresh project folder with a small `CLAUDE.md` keeps per-turn context (and cost) low.

## Plan choice (checked 2026-10-19, claude.com/pricing)
- Pro: $20/mo monthly or $17/mo billed annually; includes Claude Code and Opus; limits are rolling 5-hour sessions plus weekly caps, shared with chat. Recommended start, monthly billing.
- Max 5x from $100/mo, Max 20x higher; only if Pro limits are hit often.
- Alternative: pay-as-you-go API credits (~$9-15 for this project, no cap).

## Status
Planning only. Nothing built yet. Next step on go-ahead: verify the SDK repo branches and Era Color SoC, set up WSL2 or Docker, build and run a hello-world `.app`.
