# Codemap

Where code lives in the Tekken 3 port. Capability state is in `docs/project-state.md`, feature hops in
`docs/features.md`, the evidence chain in `docs/re-frontier.md`, atomic work in `docs/issues/`. The framework
side is `external/psxport/docs/codemap.md`.

Product path: `run.sh` → `tools/run.py` (provision, build) → the framework main (`psxport_add_product`, target
`tekken3_port`) → `psx::host::ProductHost` over `tekken3::createCatalog` → `TitleSession` → `tekken3::Runtime`
(`registerOverrides`, `startProgram`) → `tekken3::frame::FrameDriver::stepFrame` once per display field on
psxport's Lightrec executor.

Frame loop: owned. `FrameDriver` runs the main loop body (`FiniteFrame::step`, retail `0x80028BCC`) one field per step; the leading VSync(0) of `0x800B0954` is removed.

## Placement rules

- Everything is in namespace `tekken3` (sub-namespaces `frame`, `execution`, `cd`, `render`, `widescreen`,
  `field`, `loader`). `game/` is the only first-party include root: `"frame/finite_frame.h"`; psxport headers
  are bare.
- Per-Core title state lives on `tekken3::Session`, reached through `session(core)`; the runtime holds only what
  is the same for every Core (stock VSync and pad bindings, the widescreen policy).
- Guest calls from the frame go through `frame::Machine`, so the frame sequence is tested on a recorder and
  shipped on `CoreMachine`. A guest call that must return goes through `execution::FiniteGuestCall`.
- A measured address lives at a name in its owner's header with its provenance.
- psxport (`external/psxport`, a link to the live checkout) owns the executor, override keys, the stock
  VSync/pad classes and presentation. This repository supplies addresses and title behaviour.

## Owners

| Owner | Path | What it owns |
|---|---|---|
| `createCatalog` | `game/product/catalog.*` | The one-title catalog over the `SLUS_004.02` identity (generated from `titles/tekken3/executable.json`), provisioned under `scratch/bin` |
| `tekken3::Runtime` | `game/runtime/runtime.*` | The `GameRuntime`: `StockVSync` at `0x800859A8` answering from `0x8009AC68`, `StockPadBuffers`, the frame/CD/GPU/projection natives, `startProgram` into the boot prefix, Record without interpolation, the `warp` command |
| `tekken3::Session`, `session(core)` | `game/runtime/session.*` | The per-Core projection plan the widescreen overrides latch and read |
| `frame::Machine`, `FiniteFrame` | `game/frame/finite_frame.*` | The guest main-loop bodies replaced natively: boot prefix `0x80028BA0`, the two retail initializers as spanning calls, frame barrier `0x800296C4`, display init `0x800B0954` without its VSync wait, and the field step |
| `frame::FrameDriver`, `CoreMachine` | `game/frame/frame_driver.*` | One display field: logic frame, an armed warp at a boundary with no guest call in flight, then `FiniteFrame::step`; holds the `ResumableGuestCall` a mode body spans fields with |
| `tekken3::DebugMenu` | `game/debug/debug_menu.*` | The title debug menu (psxport `TitleDebugMenu`): `warp <p1> <p2>` |
| `frame::FightWarp` | `game/frame/fight_warp.*` | `warp <p1> <p2>`: the title menu's VS game start (`0x800DA910`, then `0x8007C30C(1)`), then character select's own exit (phase 3) with both choices written |
| `execution::FiniteGuestCall` | `game/execution/finite_guest_call.*` | A guest call that must return within a field, and the account of one that did not |
| `tekken3::loader` facts | `game/cd/loader_lifecycle.h` | The decoded CD-read lifecycle: wait byte, chain record, callbacks |
| `cd::installOverrides` | `game/cd/sector_ready_order.*` | Instant reads in the order the guest's libcd assumes: class-2 completion before the first sector, ISR before the INT flush |
| `render::installOverrides` | `game/render/gpu_queue_timeout.*` | The libgpu queue timeout (`0x8007E8F0`, `0x8007E924`) on the guest field word |
| `widescreen::WidescreenProjection` | `game/widescreen/widescreen.*` | `FUN_80080A40` gets the widened view; the stage and effect clippers cull at the canvas |
| `widescreen::StageTileWedge` | `game/widescreen/stage_tile_wedge.*` | `FUN_8006D95C`'s 6x6 stage-tile selection for a wedge widened in the tangent domain |
| `field::kEntry`, `kCounter`, `readCounter` | `game/fieldclock/field_clock.*` | libetc VSync and the field count a negative mode returns |
| tools | `bootstrap.py`, `tools/` | The `external/psxport` fetch (`bootstrap.py`), launcher, provisioning, RE probes |

## The frame turn

| Hop | Owner |
|---|---|
| product spine | `ProductHost` → `TitleSession`: `admitExecutable` (framework default) → `registerOverrides` → `startProgram` → `FrameDriver::runBootPrefix` |
| field | `FrameDriver::stepFrame`: `logicFrame`, `otAttr.beginLogicFrame`, warp, `FiniteFrame::step` |
| step | barrier (pad, RCntCNT2 event), presentation commit, audio, CD/XA state, buffer and geometry select, the mode body, OT splice |
| guest that spans fields | a mode body or initializer runs as a resumable call; held fields still service pad, presentation and audio |
| presentation | psxport's record path replays the guest's GP0 output; one commit per field at the game's native 60 fps |

## Input, CD, widescreen, debug

| Concern | Owner |
|---|---|
| host pad | psxport `Pad::serviceFrame` (from `Machine::servicePad` in the barrier), then `Runtime::deliverPad` |
| libcd | the guest's own; reads are instant; `sector_ready_order` keeps its completion order |
| widescreen | `Runtime::guestWidescreenProjection` → `WidescreenProjection` latches into `Session::projectionPlan`; clippers and the stage wedge read that plan; 4:3 runs the retail bodies |
| warp | `DebugMenu` (`game/debug/debug_menu.*`, psxport `TitleDebugMenu`) → `FrameDriver::armWarp` → `FightWarp::apply` at a field boundary |

## Where does it go?

| Responsibility | Owner |
|---|---|
| Composition or identity | `game/product/` |
| Framework-facing title policy | `game/runtime/` |
| Anything once per field, boot, or the debug warp | `game/frame/` |
| A guest call that must return | `game/execution/` |
| CD order or loader facts | `game/cd/` |
| GPU queue deadline | `game/render/` |
| Projection, clipping, stage-tile visibility | `game/widescreen/` |
| Guest field clock facts | `game/fieldclock/` |
| Instruction semantics, machine sync, invalidation, input, presentation | `external/psxport` |
| An executable fact | `titles/tekken3/executable.json` |
