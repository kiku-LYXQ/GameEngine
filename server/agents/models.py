from __future__ import annotations

from datetime import datetime
from enum import Enum
from typing import Dict, List, Optional

from pydantic import BaseModel


class AgentStatus(str, Enum):
    pending = "pending"
    running = "running"
    done = "done"
    failed = "failed"


class CostEstimate(BaseModel):
    tokens: int
    latency_ms: int


class AgentTaskStep(BaseModel):
    name: str
    description: str
    dependencies: List[str] = []
    cost_estimate: Optional[CostEstimate]


class AgentTaskRequest(BaseModel):
    prompt: str
    project_context: Optional[Dict[str, str]]


class AgentTaskResult(BaseModel):
    task_id: str
    status: AgentStatus
    steps: List[AgentTaskStep]
    outputs: Dict[str, str]
    started_at: datetime
    updated_at: datetime


class AgentStatusResponse(BaseModel):
    task_id: str
    status: AgentStatus
    logs: List[str]


class AgentCapability(BaseModel):
    name: str
    description: str
    success_rate: float
    avg_latency_ms: int
    avg_tokens: int


class AgentCapabilitiesResponse(BaseModel):
    capabilities: List[AgentCapability]


class TaskLogsResponse(BaseModel):
    task_id: str
    logs: List[str]


class NpcTaskRequest(BaseModel):
    npc_id: str
    behavior: str
    intent: str
    chunk_id: Optional[str]


class NpcTaskResponse(BaseModel):
    task_id: str
    dialogue: List[str]
    task_script: str
    behavior_plan: Dict[str, str]
