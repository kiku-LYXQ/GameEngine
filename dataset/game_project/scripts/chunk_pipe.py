"""示例 chunk pipeline，生成 chunk metadata + placeholder embeddings."""
from __future__ import annotations

import json
from pathlib import Path
from typing import List, Dict

from server.rag.embeddings import embed_text


def chunk_text(text: str, chunk_size: int = 512) -> List[str]:
    tokens = text.split()
    chunks = []
    for i in range(0, len(tokens), chunk_size):
        chunk = " ".join(tokens[i : i + chunk_size])
        chunks.append(chunk)
    return chunks


def emit_chunks(path: Path) -> List[Dict[str, object]]:
    text = path.read_text(encoding="utf-8", errors="ignore")
    raw_chunks = chunk_text(text)
    return [
        {
            "path": str(path),
            "chunk": chunk,
            "embedding": embed_text(chunk),
            "metadata": {
                "entity_type": "doc_section",
                "line_range": None,
                "tags": ["doc", "design"],
            },
        }
        for chunk in raw_chunks
    ]


def dump_chunks(root: Path, dest: Path) -> None:
    chunks = []
    for file in root.rglob("*.md"):
        chunks.extend(emit_chunks(file))
    dest.write_text(json.dumps(chunks, indent=2))


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Chunk text files for RAG ingestion.")
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--dest", type=Path, default=Path("chunks.json"))
    args = parser.parse_args()
    dump_chunks(args.root, args.dest)
