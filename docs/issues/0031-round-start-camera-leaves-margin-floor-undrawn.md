---
id: 31
title: At 16:9 the round-start camera leaves a floor wedge undrawn in each bottom margin corner
status: open
symptom: at the start of a fight (attract demo Eddy vs Paul, field 6,700) a triangle at the bottom of each margin shows stale canvas (white since issue 0030, black before)
state_items: S007
tags: tekken3,widescreen,stage-tiles,margins
created: 2026-11-17
updated: 2026-11-17
---

## Observed

`scratch/t3pass/cmp6700.png` (top: before issue 0030, bottom: after): the same two wedges, under the floor's near
edge at the canvas sides. Later cameras in the same fight (7,200, 7,600) fill the margins.

## Not established

Tekken draws no clear behind the stage; at 4:3 the stage covers the buffer. The missing floor is near the camera at
the widest angles, where `StageTileWedge::selectTiles` (`FUN_8006D95C`'s 6x6 selection) or the tile grid itself
stops. Next: at that frame, list the tiles `FUN_8006D95C` selects and the floor quads `FUN_8006CC28` rejects for
the canvas, and find which owner drops the corner.
