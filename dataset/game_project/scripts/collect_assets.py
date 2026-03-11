"""采集 UE 项目资产/代码作为 RAG 输入。"""
from __future__ import annotations

import json
import os
from pathlib import Path
from typing import Dict, List


def collect_files(root: Path, extensions: List[str]) -> List[Path]:
    return [p for p in root.rglob("*") if p.suffix.lower() in extensions]


def gather_metadata(path: Path) -> Dict[str, str]:
    return {
        "path": str(path.relative_to(path.anchor)),
        "size": str(path.stat().st_size),
        "modified": path.stat().st_mtime_ns,
    }


def export_manifest(root: Path, output: Path, extensions: List[str]) -> None:
    files = collect_files(root, extensions)
    manifest = [gather_metadata(p) for p in files]
    output.write_text(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Collect UE project assets for RAG input")
    parser.add_argument("--root", type=Path, required=True, help="UE project root path")
    parser.add_argument("--output", type=Path, default=Path("manifest.json"))
    parser.add_argument(
        "--extensions",
        nargs="+",
        default=[".cpp", ".h", ".uasset", ".umap", ".md"],
    )
    args = parser.parse_args()
    export_manifest(args.root, args.output, args.extensions)
