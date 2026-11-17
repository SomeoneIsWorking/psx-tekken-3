---
id: 28
title: The movie in the title/attract mode never plays; the screen is black for about 1,200 fields each time
status: open
symptom: from boot, fields ~250 to 1,442 are black, then the title menu shows before NAMCO PRESENTS; every attract cycle repeats a black stretch of the same length before the menu
state_items: S003,S008
tags: tekken3,fmv,mdec,cd-stream
created: 2026-10-29
updated: 2026-10-29
---

## Reproduction

`build/agent/bin/tekken3_port` hidden, unpaced, any aspect, `PSXPORT_DEBUG=mdecdma,fmv,cdc`, 1,500 fields. Mode 3
(`0x800DB1B8`) spans 1,219 host turns (684M guest cycles, `guest-call` line) and presents black throughout.

## Measured

Inside that call the guest sets mode `0xC0`, SetLoc 51:40:06 and ReadS (`0x1B`): a streamed movie. The drive delivers
7 sectors (`file=1 chan=1 submode=0x48`, video) and no more. MDEC DMA0 starts with 58,336 words and is deferred
("no decode command in flight"), DMA1 out waits with `canread=0`; about 3 s later both are force-stopped and abandoned,
the stream is paused and the mode continues to the menu.

## Not established

Whether the stream stops because a sector INT is lost under instant reads (the class the CD ready order in
`game/cd/sector_ready_order.cpp` handles for the loader, issues 0020, 0027) or because the MDEC decode command never
reaches the device. Retail order (movie, then title) is not confirmed against an independent emulator.

## Proper fix

Trace the stream's ready callback and the MDEC command writes from the 7th sector on, find the first divergence from the
retail order, and fix it at that owner (the title's CD order or psxport's MDEC/CD devices). No movie skip or stub.

## Update 2026-11-17

Still black from field 197 to 1,416 on HEAD (`scratch/t3pass/boot`), and Start at field 900 does nothing
(`scratch/t3pass/skipmovie`). Retail's step `0x800E1374` (phase `0x800EC528` = 2) decodes a frame (`0x800E1C48`),
shows it, parses the pad itself (`0x80029DC0(0)`) and only then tests Start in `0x800ADD68`/`0x800ADD6A` bit `0x800`;
on Start it stops the stream (`0x800E2054`, `0x800E22F0`, `0x80082B98`, `0x8006BC98(0,-1)`, CD Pause) and ends with
`0x800EC526` = -2. With no decoded frame the test is never reached, so the movie's Start skip waits on this issue.

The movie the title asks for (kind `0x800A05DC` = 3, table `0x800B9C70`, entry 0 → descriptor 21 of `0x800B981C`)
leaves its first sector in `0x800EC548` = 231,752; the drive's SetLoc is 51:40:06 (LBA 232,356), so the descriptor
is not a bare LBA. Toy Story 2 plays its guest-requested movie through psxport's native `Fmv`
(`psx-toy-story-2/game/fmv/movie_player.cpp`); doing the same here needs that mapping and the stream's channel.
