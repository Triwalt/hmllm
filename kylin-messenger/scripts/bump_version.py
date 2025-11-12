#!/usr/bin/env python3
"""Utility script to manage the project VERSION file.

Usage examples:

  python3 bump_version.py --set 2.0.0
  python3 bump_version.py --bump minor

The script updates the VERSION file in-place and prints the resulting version
string so that it can be chained inside shell pipelines if required.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys

VERSION_FILE = pathlib.Path(__file__).resolve().parent.parent / "VERSION"
VERSION_PATTERN = re.compile(r"^(\d+)\.(\d+)\.(\d+)$")


def read_version() -> tuple[int, int, int]:
    text = VERSION_FILE.read_text(encoding="utf-8").strip()
    match = VERSION_PATTERN.match(text)
    if not match:
        raise ValueError(f"VERSION 文件内容非法: '{text}'")
    return tuple(int(group) for group in match.groups())  # type: ignore[return-value]


def write_version(major: int, minor: int, patch: int) -> str:
    new_version = f"{major}.{minor}.{patch}"
    VERSION_FILE.write_text(new_version + "\n", encoding="utf-8")
    return new_version


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="管理 VERSION 文件的版本号")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--set", dest="set_version", metavar="X.Y.Z",
                       help="直接将版本号设置为指定值")
    group.add_argument("--bump", choices=["major", "minor", "patch"],
                       help="在当前版本号的基础上执行递增")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)

    if args.set_version:
        if not VERSION_PATTERN.match(args.set_version):
            raise ValueError("--set 需要传入形如 X.Y.Z 的版本号")
        major, minor, patch = (int(x) for x in args.set_version.split("."))
        new_version = write_version(major, minor, patch)
        print(new_version)
        return 0

    major, minor, patch = read_version()
    if args.bump == "major":
        major += 1
        minor = 0
        patch = 0
    elif args.bump == "minor":
        minor += 1
        patch = 0
    elif args.bump == "patch":
        patch += 1
    else:
        raise AssertionError("未知的 bump 类型")

    new_version = write_version(major, minor, patch)
    print(new_version)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

