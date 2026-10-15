#!/usr/bin/env python3
"""Run Tekken 3's complete asset-free Linux product gate."""

from __future__ import annotations

import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PSXPORT = ROOT / "external" / "psxport"


def main() -> int:
    if not (PSXPORT / "cmake" / "psxport.cmake").is_file():
        print(f"[verify] REFUSED: no framework at {PSXPORT}; run `uv run --no-project https://raw.githubusercontent.com/SomeoneIsWorking/psxport/main/tools/psxport_fetch.py --auto`", file=sys.stderr)
        return 2

    sys.path.insert(0, str(PSXPORT / "tools"))
    from port.consumer_verify import ConsumerVerifyConfig, run_consumer_verification

    build = ROOT / "build" / "ci"
    return run_consumer_verification(
        ConsumerVerifyConfig(
            name="Tekken 3",
            root=ROOT,
            build=build,
            psxport=PSXPORT,
            product=build / "bin" / "tekken3_port",
            cmake_module=ROOT / "CMakeLists.txt",
            test_regex=(
                r"^tekken3_(product_help_contract|platform_bindings_contract|fight_warp_contract|frame_driver_contract|"
                r"finite_frame_contract|bounded_guest_call|finite_guest_call_contract|mode_call_budget_resume|"
                r"sector_ready_order_contract|gpu_queue_timeout_contract|vsync_field_clock_contract|"
                r"widescreen_contract|widescreen_stage_wedge_contract|irq_oracle_selftest|"
                r"launcher_selftest|launcher_help_contract)$"
            ),
            cmake_definitions=("-DPSXPORT_BUILD_SMOKE=OFF",),
            build_targets=("cpp_policy",),
        )
    )


if __name__ == "__main__":
    raise SystemExit(main())
