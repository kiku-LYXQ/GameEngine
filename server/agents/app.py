from __future__ import annotations

import logging
from typing import Dict

from fastapi import FastAPI, BackgroundTasks, HTTPException

from .agents.asset import find_assets
from .agents.code import execute_code_agent
from .agents.doc import create_documentation
from .agents.planner import plan_from_prompt
from .context import task_context_store
from .models import (
    AgentStatus,
    AgentStatusResponse,
    AgentTaskRequest,
    AgentTaskResult,
    AgentTaskStep,
)

logger = logging.getLogger("server.agents")
app = FastAPI(title="AI GameDev Agent Orchestrator")


@app.post("/agents/task", response_model=AgentTaskResult)
def create_task(payload: AgentTaskRequest, background_tasks: BackgroundTasks) -> AgentTaskResult:
    record = task_context_store.new_task()
    record = task_context_store.update_task(record.task_id, status=AgentStatus.running, log="Planner started")
    steps = plan_from_prompt(payload.prompt)
    outputs: Dict[str, str] = {}

    for step in steps:
        outputs[step.name] = f"Pending execution for {step.name}."

    background_tasks.add_task(_run_agents, record.task_id, steps, payload.project_context)

    result = AgentTaskResult(
        task_id=record.task_id,
        status=AgentStatus.running,
        steps=steps,
        outputs=outputs,
        started_at=record.created_at,
        updated_at=record.updated_at,
    )
    return result


def _run_agents(task_id: str, steps: list[AgentTaskStep], context: Dict[str, str] | None) -> None:
    try:
        task_context_store.update_task(task_id, status=AgentStatus.running, log="Code agent executing")
        for step in steps:
            if step.name == "analysis":
                outputs = execute_code_agent(step)
            elif step.name == "code":
                outputs = execute_code_agent(step)
            elif step.name == "assets":
                outputs = ", ".join(find_assets(["explosion", "player"]))
            else:
                outputs = create_documentation(step)
            task_context_store.update_task(task_id, status=AgentStatus.running, log=f"{step.name} done")
        task_context_store.update_task(task_id, status=AgentStatus.done, log="Task completed")
    except Exception as exc:
        logger.exception("Agent task failed")
        task_context_store.update_task(task_id, status=AgentStatus.failed, log=str(exc))


@app.get("/agents/status/{task_id}", response_model=AgentStatusResponse)
def task_status(task_id: str) -> AgentStatusResponse:
    try:
        return task_context_store.get(task_id)
    except KeyError:
        raise HTTPException(status_code=404, detail="Task not found")


@app.post("/agents/feedback/{task_id}")
def submit_feedback(task_id: str, comment: str) -> dict:
    logger.info("Feedback for %s: %s", task_id, comment)
    return {"status": "received", "task_id": task_id}
