from __future__ import annotations

from typing import List


def embed_text(text: str) -> List[float]:
    """Stub embedding function."""
    # placeholder: deterministic hash-based pseudo vector
    seed = abs(hash(text)) % 1024
    return [(seed + i) % 255 / 255.0 for i in range(128)]


def similarity(vec1: List[float], vec2: List[float]) -> float:
    """Cosine similarity helper."""
    if not vec1 or not vec2:
        return 0.0
    dot = sum(a * b for a, b in zip(vec1, vec2))
    norm1 = sum(a * a for a in vec1) ** 0.5
    norm2 = sum(b * b for b in vec2) ** 0.5
    if norm1 == 0 or norm2 == 0:
        return 0.0
    return dot / (norm1 * norm2)
