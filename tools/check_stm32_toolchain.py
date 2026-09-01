#!/usr/bin/env python3
"""Check whether the local STM32 bring-up tools can be discovered."""

from __future__ import annotations

import argparse
import os
import shutil
from pathlib import Path
from typing import Iterable


TOOL_NAMES = (
    "STM32CubeIDE",
    "STM32CubeMX",
    "arm-none-eabi-gcc",
    "arm-none-eabi-g++",
    "STM32_Programmer_CLI",
    "openocd",
)


def _first_executable(candidates: Iterable[Path]) -> Path | None:
    for candidate in candidates:
        if candidate.is_file() and os.access(candidate, os.X_OK):
            return candidate
    return None


def discover_tools(home: Path | None = None) -> dict[str, Path | None]:
    """Find tools on PATH and in standard STM32CubeIDE Linux locations."""
    user_home = home if home is not None else Path.home()
    result = {name: None for name in TOOL_NAMES}

    path_names = {
        "STM32CubeIDE": "stm32cubeide",
        "STM32CubeMX": "STM32CubeMX",
        "arm-none-eabi-gcc": "arm-none-eabi-gcc",
        "arm-none-eabi-g++": "arm-none-eabi-g++",
        "STM32_Programmer_CLI": "STM32_Programmer_CLI",
        "openocd": "openocd",
    }
    for logical_name, executable_name in path_names.items():
        found = shutil.which(executable_name)
        if found is not None:
            result[logical_name] = Path(found)

    ide_roots = sorted(Path("/opt/st").glob("stm32cubeide_*"), reverse=True)
    if result["STM32CubeIDE"] is None:
        result["STM32CubeIDE"] = _first_executable(
            root / "stm32cubeide" for root in ide_roots
        )

    plugin_patterns = {
        "arm-none-eabi-gcc": (
            "plugins/com.st.stm32cube.ide.mcu.externaltools."
            "gnu-tools-for-stm32.*/tools/bin/arm-none-eabi-gcc"
        ),
        "arm-none-eabi-g++": (
            "plugins/com.st.stm32cube.ide.mcu.externaltools."
            "gnu-tools-for-stm32.*/tools/bin/arm-none-eabi-g++"
        ),
        "STM32_Programmer_CLI": (
            "plugins/com.st.stm32cube.ide.mcu.externaltools."
            "cubeprogrammer.*/tools/bin/STM32_Programmer_CLI"
        ),
        "openocd": (
            "plugins/com.st.stm32cube.ide.mcu.externaltools."
            "openocd.*/tools/bin/openocd"
        ),
    }
    for logical_name, pattern in plugin_patterns.items():
        if result[logical_name] is not None:
            continue
        candidates = (
            candidate
            for root in ide_roots
            for candidate in sorted(root.glob(pattern), reverse=True)
        )
        result[logical_name] = _first_executable(candidates)

    if result["STM32CubeMX"] is None:
        result["STM32CubeMX"] = _first_executable(
            [
                user_home
                / "STMicroelectronics"
                / "STM32Cube"
                / "STM32CubeMX"
                / "STM32CubeMX"
            ]
        )

    return result


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--show-paths",
        action="store_true",
        help="show local installation paths (do not copy them into project records)",
    )
    return parser


def main() -> int:
    args = build_parser().parse_args()
    tools = discover_tools()
    for name, path in tools.items():
        status = "FOUND" if path is not None else "MISSING"
        suffix = f" ({path})" if args.show_paths and path is not None else ""
        print(f"{name}: {status}{suffix}")
    return 0 if all(path is not None for path in tools.values()) else 1


if __name__ == "__main__":
    raise SystemExit(main())
