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

## Not established

Which primitive the record raster shades differently from the device (dither, blend or Gouraud rounding), and which
psxport change introduced it. Next: `recordcheck` with the first mismatched entry and its GP0 words on the menu frame.
