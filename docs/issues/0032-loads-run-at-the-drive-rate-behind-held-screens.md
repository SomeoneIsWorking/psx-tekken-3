---
id: 32
title: Tekken 3's loads read at the drive's rate, holding the VS screen about 4.5 s and the attract card about 2 s past their authored time
status: open
symptom: after character select the dimmed select frame holds 28 fields and the VS screen 334; the NAMCO card before the attract holds 217 fields where it authors 91
state_items: S011
tags: tekken3,loading,cd,libcd
created: 2026-11-17
updated: 2026-11-17
---

## Measured

Store watch on the mode word `0x800AE204` and phase `0x800AE224` (`PSXPORT_CW_MAX=0`, `watch 800AE204 800AE226`),
hidden and unpaced, 16:9. Boot walk `scratch/t3pass/boot2`, VS fight `scratch/t3pass/fight` (`warp 4/7` from the
menu), guest PC samples `scratch/t3pass/loadpc`.

| load | where it waits | fields | authored part |
|---|---|---|---|
| first NAMCO card's overlays | mode 2 phases 7-9 (`FUN_8004FA60`) | 90 | 91-field hold from phase 7 |
| card before the attract (mode 2 → 6) | phases 8-9 poll `0x80052A70` | 217 | 91; 126 are the load |
| menu after the movie or Start | mode 2 phases 1-4 | 2 | — |
| loader list (VS, attract) | mode 1 phase 2 | 4-6 | — |
| VS loader | mode 11 phase 1, one spanning call | 26-28, the select frame held | — |
| the two fighters | mode 8 phase 3, `FUN_8002A40C`, one spanning call | 321-334, the VS screen held | phase 4 waits 60 fields from phase 2 |

During both spanning calls every sampled PC sits in the loader's wait with `v1 = 0x800A0000`, `v0 = 1` (the held and
busy bytes `0x800A069F`/`0x800A069E`), and the drive passes 589 sectors over the 333 fields of the fighter load
(`PSXPORT_DEBUG=cdc`): the guest's own libcd reads through psxport's `CdController` at the disc rate
(`ReadTiming::Drive`, 150 sectors/s at double speed, psxport issue 0153). Nothing in those fields is CPU-bound.

## Proper fix

Remove the drive time, not the guest's waits: bind the guest's libcd reads to psxport's `GuestInterruptLibcd` with
`ReadTiming::AtOnce`, as Spyro 2/3 do (`psx-spyro/titles/spyro2/core/runtime.cpp`). That needs Tekken's libcd
entries and its own read chain (queue `0x800A3D78`, current entry `0x800A3E3C`, records at `0x800A06B8`, the
ready-order rules in `game/cd/sector_ready_order.cpp`) mapped onto the stock entries first. The card's 91 fields and
the VS screen's 60 stay; they are the screens' own time.
