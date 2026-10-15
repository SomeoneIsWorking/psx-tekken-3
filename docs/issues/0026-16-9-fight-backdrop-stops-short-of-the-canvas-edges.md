---
id: 26
title: At 16:9 a fight's backdrop stops short of both canvas edges
status: resolved
symptom: in a VS fight at 16:9 the stage floor and fighters reach the 492-wide canvas edges, but the backdrop band (rows 20..~320) leaves up to 24 columns on the left and 22 on the right black
state_items: S007
tags: tekken3,widescreen,record-path,backdrop,cull
created: 2026-10-15
updated: 2026-10-15
---

## Reproduction

`build/agent-clang/bin/tekken3_port` hidden window, `aspect=1`, `ires=1`, control channel `warp 4 7` at field
1,600, present shot at field 3,900 (Yoshimitsu vs Xiaoyu, forest stage). The display is 368x480; the canvas is
492 wide (62 columns per side).

## Cause

`game/widescreen/widescreen.cpp:WidescreenProjection::clipStagePrimitives` (the override of the stage clip
`FUN_8006CC28`) kept the retail horizontal reject: a quad was dropped when every vertex had x < 0 or none had
x < `guestDrawWidth`. On the Record path the guest keeps its 368 draw columns and the canvas adds 62 guest columns
on each side, so the reject was still the retail [0, 368). The backdrop is that clipper's output: 9-word
`POLY_FT4` (0x2C) packets with scratchpad-indexed vertices, about 35 columns wide per tile. The RAM dump of the
fight frame showed the outermost drawn tiles at x -32..1 and 351..392, each the first tile crossing 0 or 368.
`clipEffectPrimitive` (`FUN_8006E44C`) had the same bound.

## Fix

Both clippers now reject against the canvas columns,
[-(presentation width - draw width) / 2, draw width + that margin). That is [-62, 430) on Record at 16:9 and
[0, 492) on the GTE path, where the draw width itself widens. `tekken3_widescreen_contract`
(`recordCanvasMarginsKeepStageTiles`) fails with the retail bounds and passes with the canvas bounds. The same
shot now has no black column in rows 30..319 on either side.
