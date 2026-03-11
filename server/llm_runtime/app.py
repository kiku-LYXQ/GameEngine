from __future__ import annotations

import logging
import time
from uuid import uuid4

from fastapi import FastAPI, HTTPException, Query

from .lora import manager as lora_manager
from .metrics import metrics_store
from .models import (
    Choice,
    ChatCompletionRequest,
    ChatCompletionResponse,
    CompletionRequest,
    CompletionResponse,
    LoRaJobResponse,
    LoRaRequest,
    ModelInfo,
    ModelRegistryResponse,
)
from .registry import registry

logger = logging.getLogger("server.llm_runtime")
app = FastAPI(title="GameDev LLM Runtime")


@app.on_event("startup")
async def warmup() -> None:
    logger.info("Registering default models... (vLLM / Qwen)")
    registry.register(ModelInfo(name="game-qwen-7b", base_model="qwen-7b", status="ready", tags=["chat", "code"]))
    registry.register(ModelInfo(name="game-deepseek-13b", base_model="deepseek-13b", status="ready", tags=["code"]))


def _build_choice(text: str) -> Choice:
    return Choice(text=text, finish_reason="stop")


def _build_usage(prompt: str, max_tokens: int) -> dict:
    tokens = len(prompt.split())
    completion = min(max_tokens, 64)
    return {"prompt_tokens": tokens, "completion_tokens": completion, "total_tokens": tokens + completion}


@app.post("/v1/completions", response_model=CompletionResponse)
def create_completion(payload: CompletionRequest) -> CompletionResponse:
    metrics_store.increment("completions.requests")
    try:
        model_info = registry.get(payload.model)
    except KeyError:
        raise HTTPException(status_code=404, detail="Model not found")

    prompt = payload.prompt or ""
    time.sleep(0.01)
    usage = _build_usage(prompt, payload.max_tokens)
    choice = _build_choice(f"Generated completion by {model_info.name} for: {prompt[:64]}")

    response = CompletionResponse(
        id=str(uuid4()),
        model=model_info.name,
        choices=[choice],
        usage=usage,
    )
    metrics_store.increment("completions.responses")
    return response


@app.post("/v1/chat/completions", response_model=ChatCompletionResponse)
def create_chat_completion(payload: ChatCompletionRequest) -> ChatCompletionResponse:
    metrics_store.increment("chat.requests")
    try:
        model_info = registry.get(payload.model)
    except KeyError:
        raise HTTPException(status_code=404, detail="Model not found")
    prompt = "\n".join([msg.content for msg in payload.messages])
    time.sleep(0.01)
    usage = _build_usage(prompt, payload.max_tokens)
    choice = _build_choice(f"Assistant answer based on {model_info.name}")
    response = ChatCompletionResponse(
        id=str(uuid4()),
        model=model_info.name,
        choices=[choice],
        usage=usage,
    )
    metrics_store.increment("chat.responses")
    return response


@app.get("/models", response_model=ModelRegistryResponse)
def list_models(limit: int = Query(10, ge=1, le=100)) -> ModelRegistryResponse:
    models = registry.list()[:limit]
    return ModelRegistryResponse(models=models)


@app.post("/models/{model_name}/lora", response_model=LoRaJobResponse)
def request_lora(model_name: str, payload: LoRaRequest) -> LoRaJobResponse:
    metrics_store.increment("lora.requests")
    if payload.base_model != model_name:
        raise HTTPException(status_code=400, detail="Base model mismatch")
    job = lora_manager.start_job(payload)
    metrics_store.increment("lora.jobs_started")
    return job


@app.get("/metrics")
def metrics() -> dict:
    return metrics_store.snapshot()
