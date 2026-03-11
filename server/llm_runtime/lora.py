from __future__ import annotations

from datetime import datetime
from uuid import uuid4

from .models import LoRaJobResponse, LoRaRequest


class LoRaManager:
    def __init__(self) -> None:
        self._jobs: dict[str, LoRaJobResponse] = {}

    def start_job(self, payload: LoRaRequest) -> LoRaJobResponse:
        job_id = str(uuid4())
        response = LoRaJobResponse(
            job_id=job_id,
            status="queued",
            model=f"{payload.base_model}-lora",
            started_at=datetime.utcnow(),
        )
        self._jobs[job_id] = response
        return response

    def get_job(self, job_id: str) -> LoRaJobResponse:
        return self._jobs[job_id]


manager = LoRaManager()
