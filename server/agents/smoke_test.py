"""Smoke test exercising Agent -> LLM Runtime flow."""
from __future__ import annotations

import json
import time
from pathlib import Path

import httpx

BASE_URL = "http://127.0.0.1:7000"
LLM_URL = "http://127.0.0.1:7001"


def post_task(prompt: str) -> str:
    payload = {"prompt": prompt, "project_context": {"module": "Gameplay"}}
    with httpx.Client(timeout=10) as client:
        response = client.post(f"{BASE_URL}/agents/task", json=payload)
        response.raise_for_status()
        return response.json()["task_id"]


def poll_status(task_id: str, max_attempts: int = 5) -> dict:
    with httpx.Client(timeout=10) as client:
        for attempt in range(max_attempts):
            response = client.get(f"{BASE_URL}/agents/status/{task_id}")
            response.raise_for_status()
            payload = response.json()
            print(f"Attempt {attempt}: status={payload['status']}, logs={payload['logs']}")
            if payload["status"] in ("done", "failed"):
                return payload
            time.sleep(1)
        raise SystemExit("agent task did not finish in time")


def fetch_metrics() -> dict:
    with httpx.Client(timeout=5) as client:
        response = client.get(f"{LLM_URL}/metrics")
        response.raise_for_status()
        return response.json()


def run_smoke_test() -> None:
    print("Posting agent task...")
    task_id = post_task("Implement a sprint ability using RAG context.")
    status = poll_status(task_id)
    print("Final status:", json.dumps(status, indent=2))
    metrics = fetch_metrics()
    print("LLM metrics:", json.dumps(metrics, indent=2))


if __name__ == "__main__":
    run_smoke_test()
