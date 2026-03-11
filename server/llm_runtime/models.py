from __future__ import annotations

from datetime import datetime
from typing import Dict, List, Optional
from uuid import uuid4

from pydantic import BaseModel


class Choice(BaseModel):
    text: Optional[str]
    finish_reason: Optional[str]


class Usage(BaseModel):
    prompt_tokens: int
    completion_tokens: int
    total_tokens: int


class CompletionRequest(BaseModel):
    model: str
    prompt: str
    max_tokens: int = 256
    temperature: float = 0.3


class CompletionResponse(BaseModel):
    id: str
    object: str = "text_completion"
    model: str
    choices: List[Choice]
    usage: Usage


class ChatMessage(BaseModel):
    role: str
    content: str


class ChatCompletionRequest(BaseModel):
    model: str
    messages: List[ChatMessage]
    temperature: float = 0.2
    max_tokens: int = 512


class ChatCompletionResponse(BaseModel):
    id: str
    object: str = "chat.completion"
    model: str
    choices: List[Choice]
    usage: Usage


class ModelInfo(BaseModel):
    name: str
    base_model: str
    status: str
    tags: List[str] = []


class LoRaRequest(BaseModel):
    base_model: str
    dataset_path: str
    epochs: int = 2
    learning_rate: float = 3e-4


class LoRaJobResponse(BaseModel):
    job_id: str
    status: str
    model: str
    started_at: datetime


class ModelRegistryResponse(BaseModel):
    models: List[ModelInfo]


class ModelEvaluationRequest(BaseModel):
    prompt: str
    expected: Optional[str]


class ModelEvaluationResponse(BaseModel):
    model: str
    score: float
    details: Dict[str, float]
