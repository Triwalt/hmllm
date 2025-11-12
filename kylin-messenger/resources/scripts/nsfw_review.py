#!/usr/bin/env python3
"""
NSFW image classification helper for Kylin Messenger.

This script is a thin wrapper around the `nsfw_detector` package that is part of
the open-source project hosted at https://github.com/GantMan/nsfw_model (MIT
License). Please ensure the repository's license requirements are followed when
distributing this script and the associated model files.
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path


def _ensure_local_site() -> Path:
    """Ensure a writable site-packages directory for bundled installs."""

    root_override = os.environ.get("KYLIN_NSFW_PYTHON_SITE")
    if root_override:
        site_dir = Path(root_override).expanduser()
    else:
        if sys.platform.startswith("win"):
            base = Path(os.environ.get("APPDATA", Path.home() / "AppData" / "Roaming"))
            site_dir = base / "KylinMessenger" / "python"
        else:
            base = Path.home() / ".local" / "share"
            site_dir = base / "KylinMessenger" / "python"

    site_dir.mkdir(parents=True, exist_ok=True)
    if str(site_dir) not in sys.path:
        sys.path.insert(0, str(site_dir))
    return site_dir


def _auto_install_nsfw_detector(site_dir: Path) -> bool:
    """Attempt to install nsfw-detector into *site_dir* using pip."""

    auto_install = os.environ.get("KYLIN_NSFW_AUTOINSTALL", "1")
    if auto_install.lower() in {"0", "false", "no"}:
        return False

    try:
        subprocess.check_call(
            [
                sys.executable,
                "-m",
                "pip",
                "install",
                "--upgrade",
                "--no-cache-dir",
                "--target",
                str(site_dir),
                "nsfw-detector",
            ],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        return True
    except Exception:
        return False


def _load_predict_module():
    site_dir = _ensure_local_site()
    try:
        from nsfw_detector import predict  # type: ignore
    except Exception as exc:  # pragma: no cover - import error detail propagated
        if not _auto_install_nsfw_detector(site_dir):
            raise RuntimeError(
                "Unable to import nsfw_detector. Install it via 'pip install nsfw-detector'"
            ) from exc
        try:
            from nsfw_detector import predict  # type: ignore
        except Exception as exc_retry:  # pragma: no cover
            raise RuntimeError(
                "自动安装 nsfw-detector 失败，请检查网络或手动安装。"
            ) from exc_retry
    return predict


def classify_image(model_path: Path, image_path: Path) -> dict[str, float]:
    predict = _load_predict_module()
    model = predict.load_model(str(model_path))
    scores = predict.classify(model, str(image_path))
    if str(image_path) not in scores:
        raise RuntimeError("Classifier did not return result for the requested image")
    result = scores[str(image_path)]
    # Ensure numeric values (JSON serialisable floats)
    return {label: float(value) for label, value in result.items()}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run nsfw_detector on a single image")
    parser.add_argument("--model", required=True, help="Path to the nsfw_detector model (.h5)")
    parser.add_argument("--image", required=True, help="Path to the image to classify")
    parser.add_argument(
        "--pretty", action="store_true", help="Pretty-print JSON output for readability"
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    model_path = Path(args.model)
    image_path = Path(args.image)

    if not model_path.is_file():
        print(json.dumps({"error": f"Model not found: {model_path}"}), file=sys.stderr)
        return 2
    if not image_path.is_file():
        print(json.dumps({"error": f"Image not found: {image_path}"}), file=sys.stderr)
        return 3

    try:
        result = classify_image(model_path, image_path)
    except Exception as exc:  # pragma: no cover - error propagated to caller
        print(json.dumps({"error": str(exc)}), file=sys.stderr)
        return 4

    dump = json.dumps(result, indent=2 if args.pretty else None)
    print(dump)
    return 0


if __name__ == "__main__":  # pragma: no cover - manual invocation
    sys.exit(main())

