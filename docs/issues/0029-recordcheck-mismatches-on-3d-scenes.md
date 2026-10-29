---
id: 29
title: recordcheck reports thousands of mismatched pixels on the 3D scenes and fights at 4:3
status: open
symptom: `recordcheck` mismatched > 0 on fields 1,442-1,926 (title menu), 2,610-6,151 (intro scenes) and every demo fight, at 4:3 and 16:9; issue 0024 had 0 on all of them
state_items: S005
tags: tekken3,record-path,recordcheck
created: 2026-10-29
updated: 2026-10-29
---

## Measured

Walk from boot at 4:3, `PSXPORT_DEBUG=recordcheck` (`scratch/readme-media/walk43b`). Title menu: 1,517-3,224 pixels,
first at (102,316), record `0x0C00` against device `0x1000`. Demo fight at 7,500: 89,042 pixels, first (3,20), record
`0x3CE4` against device `0x4126`. Every difference seen is one or two 5-bit steps per channel; the pictures look the
same to the eye.

## Cause

Dither. Since psxport `f977a8ca` (2026-10-20) the record raster does not dither and the device does, so `recordcheck`
counts every pixel of a dithered draw (E1 bit 9) as mismatched by design (psxport `docs/presentation.md`); issue 0024's
zero counts predate that change. Not psxport `e83b441c` or `131ee4db`: these presents are `composed=0`, the plain record
replay, which neither touches. Shown on HEAD with the same 1,700-field 4:3 boot (`scratch/review-fixes/{dith,nodith}`):
as shipped, 259 of 1,701 presents mismatch (title menu 3,264 pixels at (102,316)); with the device's dither switched off
under gdb (`set var psx_gpu_dither_mode = DITHER_OFF`), 11 of 1,701.

## Not established

The 11 left are title-menu presents 1,442-1,452 with 4-66 pixels on rows 360-361, the record one step darker in every
channel (`0x0000` against `0x0421`, `0x3def` against `0x4210`): a rounding difference in one draw on those rows, not
traced. Next: the entry covering (169,360) at present 1,447 and its GP0 words.
