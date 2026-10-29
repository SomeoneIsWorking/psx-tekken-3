---
id: 27
title: The attract demo's load stalls black forever when a sector of the stopping stream arrives after the read is armed
status: resolved
symptom: from boot with no input, after the PUSH START title (about field 6,600) the screen is black for good; mode 1 phase 2, `0x800A069F` = 1, the 0x4500-byte read to `0x800B0548` (ReadN at 55:54:71, LBA 251,471) gets no sector
state_items: S003
tags: tekken3,cd-completion,attract,sector-ready-order
created: 2026-10-29
updated: 2026-10-29
---

## Reproduction

`build/agent/bin/tekken3_port` hidden, unpaced, 4:3 or 16:9, present shots every 250 fields to 15,000: black from
field 6,750 to the end. RAM at the stall: mode `0x800AE204` = 1, phase `0x800AE224` = 2, held `0x800A069F` = 1, busy
`0x800A069E` = 0, remaining `0x800A06A4` = 0x4500 (no sector consumed), sector callback flag `0x8009B8E8` = 1.

## Cause

`game/cd/sector_ready_order.cpp:completeReadBeforeSector` delivered the class-2 completion for any data INT that came
while the held byte was set and no sector callback was installed. `FUN_8006C084` sets the held byte before it submits
its read chain (Pause, SetMode, SetLoc, ReadN), so the stream the attract was playing (file 1, LBA 121,304) still had a
sector in flight. That sector's INT1 installed the loader's sector callback early and fed it the stream's sector
(`cdc,cdcw` trace: `ready a0=1 held=1 flag=0` right after the chain's first Pause). The chain's real class 2 then found
the callback already registered, and the first ReadN sector was discarded by the next command's INT flush.

## Fix

The early completion applies only once the held chain's read command is the drive's current one: the chain queue's
current entry (`0x800A3E3C` into the 8 x 0x18-byte queue at `0x800A3D78`, command byte at +4) is ReadN or ReadS and is
the chain's last entry (`loader_lifecycle.h`, `FUN_8008F08C`/`FUN_8008E928`). A sector before that is the stopping
stream's and goes through the retail ready hook untouched.

## Shown

Same run, shots every 500 fields to 20,000 (4:3): the attract now runs the demo fight (Eddy vs King, field 7,500), the
title menu, a second demo (Paul vs Hwoarang), the GREATEST SURVIVORS ranking and the intro scenes again, `faults=0`.
