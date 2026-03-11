from __future__ import annotations

from typing import List, Optional
from pydantic import BaseModel, Field


class ChunkMetadata(BaseModel):
    path: str
    entity_type: str
    module: Optional[str]
    line_range: Optional[List[int]]
    tags: List[str] = Field(default_factory=list)
    hash: Optional[str]
    project_version: Optional[str]


class Chunk(BaseModel):
    id: str
    text: str
    metadata: ChunkMetadata


class RagQueryRequest(BaseModel):
    question: str
    context_tags: List[str] = Field(default_factory=list)
    max_results: int = 5
    project_version: Optional[str]


class RagQueryResponse(BaseModel):
    chunks: List[Chunk]


class RagContextRequest(BaseModel):
    entity_id: str


class RagContextResponse(BaseModel):
    chunk: Chunk


class RagRefreshRequest(BaseModel):
    project_version: Optional[str]
