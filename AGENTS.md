# Tekken 3 port

Tekken 3 (`SLUS_004.02`) ships as one native/Lightrec psxport product; the authenticated executable
is runtime data. Framework contract: `external/psxport/AGENTS.md`; framework changes:
`external/psxport/docs/workspace/PROTOCOL.md`. Execution policy: the `dynarec-port` and
`dynarec-overrides` skills.

## Execution

psxport owns machine-state synchronization, HLE/device callbacks, override invalidation,
executable-memory invalidation, and bounded exits; Lightrec owns its code cache and executable
memory. A release threshold on Lightrec's interpreted-block fallback must fail loudly.

The first discriminator is `NAMCO PRESENTS` within 1,200 frames with nonzero Lightrec execution and
all 14 address-based original calls routed through the shipping dispatcher.

## Product and enhancement boundaries

Tekken 3 already runs at 60 fps. Its rendering-enhancement scope is widescreen only: do not add an
fps60 mode, interpolation/lerp, or temporal state maintained solely for interpolation. Bind the
measured guest camera/projection/culling owners through the shared non-temporal guest-widescreen
contract. Do not add a title-owned native renderer, and never reconstruct pictures from GTE/OT/GP0
diagnostic output.

## Layout

`game/runtime/runtime.*` (`tekken3::Runtime`, a `GameRuntime`) is the one process-lifetime game
owner and `game/runtime/session.*` holds per-Core state, `game/frame/` owns the finite title frame,
its measured service order and the `warp` debug route, and `game/product/catalog.*` hands the title
to the framework main, whose `ProductHost` and `TitleSession` own the command line, boot and field
loop. `game/` is the only first-party include root. Title behaviour is a `Runtime` virtual or a
stock psxport class bound in `registerOverrides`, never a config bag; Tekken source must not include
`LegacyGameRuntimeAdapter`, `GameConfig` or `GameHooks`.

`external/psxport` is a relative symlink to the workspace's live framework checkout when one exists,
or a clone of its main where there is none. psxport's `tools/psxport_fetch.py`, run from psxport
`main` by `tools/run.py`, establishes whichever applies; there is no framework pin, so a framework
edit is immediately this game's framework. Framework edits happen in the workspace's `psxport`,
never here.

`./run.sh` hands off to `bootstrap.py` and `tools/run.py`, which provision and identity-check the
user's disc executable, build `tekken3_port`, and launch it. CMake owns compiler discovery; the
launcher adds no compiler-identity allowlists, denylists, or forced compiler selections.
