from __future__ import annotations

from typing import Dict, List
from uuid import uuid4

from .models import Chunk, ChunkMetadata


class VectorStore:
    def __init__(self):
        self._store: Dict[str, Chunk] = {}

    def add_chunk(self, text: str, metadata: ChunkMetadata) -> Chunk:
        chunk_id = str(uuid4())
        chunk = Chunk(id=chunk_id, text=text, metadata=metadata)
        self._store[chunk_id] = chunk
        return chunk

    def list_chunks(self) -> List[Chunk]:
        return list(self._store.values())

    def get_chunk(self, chunk_id: str) -> Chunk:
        return self._store[chunk_id]

    def query(self, tags: List[str], project_version: str | None, max_results: int) -> List[Chunk]:
        candidates = list(self._store.values())
        if tags:
            candidates = [c for c in candidates if set(tags) & set(c.metadata.tags)]
        if project_version:
            candidates = [c for c in candidates if c.metadata.project_version == project_version]
        return candidates[:max_results]
