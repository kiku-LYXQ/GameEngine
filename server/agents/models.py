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


class AgentTaskStep(BaseModel):
    name: str
    description: str
    dependencies: List[str] = []


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
