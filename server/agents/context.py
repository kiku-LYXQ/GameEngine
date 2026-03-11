from __future__ import annotations

from dataclasses import dataclass, field
from datetime import datetime
from typing import Dict, List
from uuid import uuid4

from .models import AgentStatus, AgentStatusResponse, TaskLogsResponse


@dataclass
class TaskRecord:
    task_id: str
    status: AgentStatus
    logs: List[str] = field(default_factory=list)
    created_at: datetime = field(default_factory=datetime.utcnow)
    updated_at: datetime = field(default_factory=datetime.utcnow)


class ContextStore:
    def __init__(self):
        self._store: Dict[str, TaskRecord] = {}

    def new_task(self) -> TaskRecord:
        task_id = str(uuid4())
        record = TaskRecord(task_id=task_id, status=AgentStatus.pending)
        self._store[task_id] = record
        return record

    def update_task(self, task_id: str, *, status: AgentStatus, log: str | None = None) -> TaskRecord:
        record = self._store[task_id]
        record.status = status
        record.updated_at = datetime.utcnow()
        if log:
            record.logs.append(log)
        return record

    def _get_record(self, task_id: str) -> TaskRecord:
        record = self._store.get(task_id)
        if not record:
            raise KeyError(task_id)
        return record

    def get(self, task_id: str) -> AgentStatusResponse:
        record = self._get_record(task_id)
        return AgentStatusResponse(task_id=record.task_id, status=record.status, logs=list(record.logs))

    def logs(self, task_id: str) -> TaskLogsResponse:
        record = self._get_record(task_id)
        return TaskLogsResponse(task_id=record.task_id, logs=list(record.logs))


task_context_store = ContextStore()
