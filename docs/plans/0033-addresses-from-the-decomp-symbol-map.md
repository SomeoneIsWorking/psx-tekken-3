# 0033 — Guest addresses from the decomp symbol map

The port reads guest addresses from the decomp's symbol map as generated constants, never a hand copy. Tekken 3's
decomp is `external/tekken3-decomp` (MIT, ours); its map is `config/symbols/slus_004.02.txt`, splat
`symbol_addrs` format. Overlay names wait in `docs/unplaced-overlay-names.md` there until the `TEKKEN3.BNS`
logical images have their own configs.

## Today

- 98 `constexpr ... k<Name> = 0x80xxxxxx` in 11 files under `game/`, plus the mode table literals in
  `game/frame/finite_frame.cpp` and the pad buffers in `game/runtime/runtime.cpp`. Every one has its name in the
  map (seeded 2026-11-17), spelled the map's way (`kFrameBarrier` is `FrameBarrier`, `kRenderMode` is `g_Mode`).
- Return addresses passed to guest calls (`{kSelectBuffer, 0x80028C04u}`) are code sites, not symbols.

## Generator

The shared psxport tool and CMake function of psx-tomba plan 0065 (`tools/symbol_constants.py`,
`psxport_symbol_constants(<target> IMAGE <name> MAP <file>...)`), emitting `tekken3::sym::slus_004_02`. Tekken adds
no second parser. Code sites become `<function> + 0xNN` against a map symbol.

## Order

1. Wire `psxport_symbol_constants` for `tekken3_port` once the tool exists (after Tomba! 2's step 1).
2. Convert one subsystem per commit: `game/frame` (with `finite_frame.cpp`'s mode table), `game/cd`,
   `game/fieldclock`, `game/execution`, `game/render`, `game/widescreen`, `game/runtime`. A constant the map lacks
   is added to the decomp first (decomp commit, submodule bump in the same port change).
3. Place the overlay names (extract the BNS logical images, one splat config each, committed to the decomp), then
   convert the overlay addresses in `game/frame/fight_warp.h`.
4. Build with Clang; boot to the fight through the VS warp once to show no binding moved.

Until it lands, a new address goes into the decomp map first, then into its owner's header under the same name.
