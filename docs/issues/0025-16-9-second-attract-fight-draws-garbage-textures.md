---
id: 25
title: At 16:9 the second attract fight draws garbage textures where 4:3 shows a clean fight
status: open
symptom: after the first attract fight (field 12,000, clean at 16:9) the next scene at about field 12,500 and 13,000 is noise textures on the stage and both characters; 4:3 at the same fields is clean
state_items: S005,S007
tags: tekken3,widescreen,record-path,textures
created: 2026-10-10
updated: 2026-10-29
---

## Observed

`build/bin/tekken3_port` headless, `PSXPORT_DEBUG=recordcheck`, shots at 7,000 to 12,500 and 13,000 to 13,001.
At 16:9 the scenes through field 12,000 are clean (time record, first attract fight). At 12,500 and 13,000 the
fight scene is garbage textures. At 4:3 the attract timeline differs (another matchup at 12,500, Hwoarang) and is clean.
`recordcheck` is mismatched=0 on every present at both aspects, so the record path agrees with the device and the
garbage is in the device's VRAM: a guest or title state difference, not the presenter.

## Not established

Whether the 16:9 view width handed to the guest (`WidescreenProjection::publishDimensions`, 512) changes what the
guest uploads or clears in the texture area right of the 368-wide buffer, or the CD-loaded texture set differs because
the attract timeline diverged. Next: dump VRAM at field 12,500 for both aspects and find the first texture upload that
differs.

## Update 2026-10-29

Since issue 0027 let the attract loop run, the corruption starts with the first demo: walk from boot at 16:9, shots every
500 fields to 20,000 (`scratch/readme-media/walk169c`). The VS screen at field 7,000 shows VRAM texture pages (a font
sheet, a portrait strip) in both margins; the Eddy vs King demo at 7,500 and every later demo, the ranking and the next
VS screen draw striped and wrong textures over the whole picture; the PUSH START title and the logo card have white
margins. 4:3 at the same fields is clean. `recordcheck` at 7,500: `resynced=31` at 16:9 against 1 at 4:3.

Ruled out by changing one owner per run: the 512 view width handed to `FUN_80080A40` (the guest's view kept at 384:
same pictures) and the widened stage wedge (`StageTileWedge::selectTiles` on the retail body: same pictures). A VS
fight reached through `warp` (Yoshimitsu vs Xiaoyu, Lei vs Jin) is clean at 16:9 to its time-up.

Next: dump the record VRAM image and the device VRAM at the first resync after field 6,600 at 16:9 and find the first
write that differs; the record path's canvas (psxport `RecordRasterizer`) should never reach the VRAM image.
