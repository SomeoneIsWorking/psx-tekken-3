# Features — Tekken 3

Title hops only. Each row is where Tekken 3 joins a psxport feature hop (`psxport F-X.n`, psxport
`docs/features.md`); a feature with no row runs on the framework alone.

## F-BOOT

| hop | joins | owner |
|---|---|---|
| 1 | psxport F-BOOT.1 | `game/product/catalog.cpp:createCatalog` — the `SLUS_004.02` identity and a `tekken3::Runtime` |
| 2 | psxport F-BOOT.5 | `game/runtime/runtime.cpp:Runtime::startProgram` — `FrameDriver::runBootPrefix` from the admitted entry |
| 3 | psxport F-BOOT.6 | `game/runtime/runtime.cpp:Runtime::registerOverrides` — `StockVSync` `0x800859A8`, the frame, CD, GPU-timeout and projection natives |
| 4 | psxport F-BOOT.6 | `game/runtime/session.cpp:Session` — the per-Core projection plan |

## F-TURN

| hop | joins | owner |
|---|---|---|
| 1 | psxport F-TURN.2 | `game/frame/frame_driver.cpp:FrameDriver::stepFrame` — logic frame, an armed warp, then `FiniteFrame::step` |
| 2 | psxport F-TURN.2 | `game/frame/finite_frame.cpp:FiniteFrame::step` — the native main-loop body; mode bodies run as spanning calls |

## F-INPUT

| hop | joins | owner |
|---|---|---|
| 1 | psxport F-INPUT.1 | `game/runtime/runtime.cpp:Runtime::deliverPad` — `StockPadBuffers` at `0x800A9132`/`0x800A915C` |

## F-CD

| hop | joins | owner |
|---|---|---|
| 1 | psxport F-CD.1 | `game/cd/sector_ready_order.cpp:installOverrides` — the linked libcd ready callback order |

## F-RENDER

| hop | joins | owner |
|---|---|---|
| 1 | psxport F-BOOT.6 | `game/render/gpu_queue_timeout.cpp:installOverrides` — the libgpu queue timeout on the guest field word |

## F-WIDE

| hop | joins | owner |
|---|---|---|
| 1 | psxport F-WIDE.1 | `game/widescreen/widescreen.cpp:WidescreenProjection::publishDimensions` — `FUN_80080A40` gets the widened view width |
| 2 | psxport F-WIDE.2 | `game/widescreen/widescreen.cpp:WidescreenProjection::clipStagePrimitives` — `FUN_8006CC28` culls at the canvas |
| 3 | psxport F-WIDE.2 | `game/widescreen/widescreen.cpp:WidescreenProjection::clipEffectPrimitive` — `FUN_8006E44C` culls at the canvas |
| 4 | psxport F-WIDE.2 | `game/widescreen/stage_tile_wedge.cpp:StageTileWedge::selectTiles` — `FUN_8006D95C`'s wedge widened in the tangent domain |

## F-DEBUG

| hop | joins | owner |
|---|---|---|
| 1 | psxport F-DEBUG.1 | `game/runtime/runtime.cpp:Runtime::titleCommand` — `warp <p1> <p2>` |
| 2 | psxport F-DEBUG.1 | `game/frame/fight_warp.cpp:FightWarp::apply` — the menu's VS start (`0x800DA910`), then character select's own exit |
