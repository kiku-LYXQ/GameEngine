from __future__ import annotations

import logging
from typing import List

from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware

from .embeddings import embed_text, similarity
from .models import (
    Chunk,
    ChunkMetadata,
    RagContextRequest,
    RagContextResponse,
    RagQueryRequest,
    RagQueryResponse,
    RagRefreshRequest,
)
from .vector_store import VectorStore

logger = logging.getLogger("server.rag")
app = FastAPI(title="GameDev RAG Knowledge Base")
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

vector_store = VectorStore()


@app.on_event("startup")
async def init_data() -> None:
    logger.info("Initializing RAG knowledge store...")
    metadata = ChunkMetadata(
        path="Source/Player/PlayerCharacter.cpp",
        entity_type="cpp_function",
        module="Gameplay",
        line_range=[100, 140],
        tags=["player", "movement"],
        hash="builtin",
        project_version="main",
    )
    vector_store.add_chunk("void MoveForward(float Value)", metadata)


@app.post("/rag/query", response_model=RagQueryResponse)
def query_rag(payload: RagQueryRequest) -> RagQueryResponse:
    logger.debug("Query received: %s", payload.json())
    matches: List[Chunk] = vector_store.query(
        tags=payload.context_tags,
        project_version=payload.project_version,
        max_results=payload.max_results,
    )
    if not matches:
        raise HTTPException(status_code=404, detail="No matching chunks found")
    return RagQueryResponse(chunks=matches)


@app.post("/rag/context", response_model=RagContextResponse)
def context_rag(payload: RagContextRequest) -> RagContextResponse:
    try:
        chunk = vector_store.get_chunk(payload.entity_id)
    except KeyError:
        raise HTTPException(status_code=404, detail="Chunk not found")
    return RagContextResponse(chunk=chunk)


@app.post("/rag/refresh")
def refresh_rag(payload: RagRefreshRequest) -> dict:
    logger.info("Refreshing vectors for project %s", payload.project_version)
    # placeholder: would re-embed all content in real pipeline
    return {"status": "refresh queued", "project_version": payload.project_version}
