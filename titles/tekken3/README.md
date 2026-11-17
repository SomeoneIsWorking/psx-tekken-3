# Tekken 3

The measured target, startup structure and display/projection ownership moved to the decomp:
`external/tekken3-decomp/docs/notes/executable.md` and `docs/notes/display-projection.md` there.

## Reproduce the identity measurement

After the root README's Clang configure, run the project-owned provisioner:

```sh
CCACHE_DISABLE=1 cmake --build build --target discdump
python3 tools/provision_executable.py "/path/to/disc.chd"
```

Resolution is CLI argument > `PSXPORT_TEKKEN3_DISC` > `.env` > one root `*.chd` drop-in. A selected
path that does not exist refuses rather than falling through to another disc, and ambiguous drop-ins
also refuse. No disc-derived file belongs in git. The independent CPU continues through the measured
context-save path and stops before executing the BIOS call at `0x80085DE4`. Independent execution
still needs an independently
sourced B(19) HookEntryInt model. An older unbounded trace left mapped title text at
`pc=0x000000B0`, `t1=0x19`, `ra=0x80085DEC`; its later `0xFFFF8C94` access is garbage execution,
not a hardware frontier (issue #10). These gates do not establish later initialization, that a
Tekken 3 port boots a frame, or that Lightrec reaches gameplay.
