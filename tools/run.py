#!/usr/bin/env python3
"""Provision, build, and launch the Tekken 3 native/Lightrec product."""

from __future__ import annotations

import os
import sys
from collections.abc import Sequence
from pathlib import Path

from port import launcher
from port.launch_environment import player_environment

ROOT = Path(__file__).resolve().parents[1]
EXECUTABLE = ROOT / "scratch" / "bin" / "tekken3" / "SLUS_004.02"
DISC_VARIABLE = "PSXPORT_TEKKEN3_DISC"


def launch(disc: str | None, prepare_only: bool) -> None:
    launcher.preflight()
    discdump = launcher.build_discdump(ROOT, os.environ)
    environment = {**os.environ, "PSXPORT_DISCDUMP": str(discdump)}
    launcher.run([sys.executable, "-B", "tools/provision_executable.py", *([disc] if disc else [])],
                 environment, cwd=ROOT)
    build = launcher.build_product(ROOT, "tekken3_port", os.environ)
    if prepare_only:
        launcher.say("Tekken 3 is built and ready.")
        return
    env = player_environment(os.environ, product="tekken3")
    if disc:
        env[DISC_VARIABLE] = str(Path(disc).expanduser().resolve())
    launcher.exec_product(build / "bin" / "tekken3_port", [str(EXECUTABLE)], env, ROOT)


def main(argv: Sequence[str] | None = None) -> int:
    args = launcher.argument_parser(__doc__).parse_args(argv)
    return launcher.main(lambda: launch(args.disc, args.prepare_only))


if __name__ == "__main__":
    raise SystemExit(main())
