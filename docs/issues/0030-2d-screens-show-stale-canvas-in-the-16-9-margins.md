---
id: 30
title: At 16:9 the 2D screens show stale canvas in their margins (white beside PUSH START, VRAM pages beside the VS screen)
status: resolved
symptom: the PUSH START title (mode 6 phase 6) has white margins and the VS screen shows texture pages in both margins; their pictures are 4:3 2D layouts
state_items: S005,S007
tags: tekken3,widescreen,record-path,margins,2d
created: 2026-11-17
updated: 2026-11-17
---

## Reproduction

Boot walk at 16:9, no input, shot at field 6,000 (`scratch/t3pass/boot/006000.png`): white columns beside the title.

## Cause

`game/widescreen/widescreen.cpp:WidescreenProjection` never answered `pictureBeyondBuffer`, so every frame claimed
picture in both margins. Only the stage clipper (`FUN_8006CC28`, issue 0026) draws there; a 2D screen draws nothing
in the margins, which keep what the canvas last held: the title's additive fade from white (`FUN_8004E000`, level
above 256) piling up where nothing redraws, or the VRAM the canvas was seeded from.

## Fix

`widescreen::MarginPicture` (on `Session`): the stage clip override marks the mode body drawing, the frame driver
latches it when the body has returned, and `pictureBeyondBuffer` answers the frame being sealed: unbounded after a
body that ran the stage clipper, no picture otherwise, so the presenter pillarboxes 2D screens.

## Shown

Same walk (`scratch/t3pass/boot2_sheet.png`): the title at 6,000 has black margins; the NAMCO card, the menu and the
movie stretch stay pillarboxed; the intro scenes and the demo fight still fill the canvas.
