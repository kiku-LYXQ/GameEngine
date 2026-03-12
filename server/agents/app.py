from __future__ import annotations

import logging
from typing import Dict

from fastapi import FastAPI, BackgroundTasks, HTTPException, Query

from .agents.asset import find_assets
from .agents.behavior_spec import BehaviorArchetype, BehaviorSpec
from .agents.behavior_spec_generator import BehaviorSpecGenerator
from .agents.code import execute_code_agent
from .agents.doc import create_documentation
from .agents.npc_agent import plan_npc_task
from .agents.planner import plan_from_prompt
from .context import task_context_store
from .models import (
    AgentCapability,
    AgentCapabilitiesResponse,
    AgentStatus,
    AgentStatusResponse,
    AgentTaskRequest,
    AgentTaskResult,
    AgentTaskStep,
    NpcTaskRequest,
    NpcTaskResponse,
    TaskLogsResponse,
    CopilotAssetSearchRequest,
    CopilotAssetSearchResponse,
    CopilotExplainRequest,
    CopilotExplainResponse,
    CopilotFile,
    CopilotGenerateRequest,
    CopilotGenerateResponse,
)

logger = logging.getLogger("server.agents")
app = FastAPI(title="AI GameDev Agent Orchestrator")

CAPABILITIES = [
    AgentCapability(
        name="Planner",
        description="Analyzes prompts and fetches project context.",
        success_rate=0.95,
        avg_latency_ms=180,
        avg_tokens=80,
    ),
    AgentCapability(
        name="Code",
        description="Generates C++/Blueprint artifacts with contextual scaffolds.",
        success_rate=0.92,
        avg_latency_ms=420,
        avg_tokens=190,
    ),
    AgentCapability(
        name="Asset",
        description="Locates assets via Vector Asset Index + Content browser metadata.",
        success_rate=0.9,
        avg_latency_ms=210,
        avg_tokens=130,
    ),
    AgentCapability(
        name="Doc",
        description="Synthesizes design docs and behavior notes.",
        success_rate=0.98,
        avg_latency_ms=230,
        avg_tokens=100,
    ),
    AgentCapability(
        name="NPC",
        description="Generates NPC dialogue, tasks, and behavior plans.",
        success_rate=0.9,
        avg_latency_ms=480,
        avg_tokens=220,
    ),
]


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
            task_context_store.update_task(
                task_id,
                status=AgentStatus.running,
                log=f"{step.name} done: {outputs}",
            )
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


@app.get("/agents/capabilities", response_model=AgentCapabilitiesResponse)
def get_capabilities() -> AgentCapabilitiesResponse:
    return AgentCapabilitiesResponse(capabilities=CAPABILITIES)


@app.get("/agents/logs/{task_id}", response_model=TaskLogsResponse)
def task_logs(task_id: str) -> TaskLogsResponse:
    try:
        return task_context_store.logs(task_id)
    except KeyError:
        raise HTTPException(status_code=404, detail="Task not found")


@app.get("/agents/behavior-spec/{archetype}", response_model=BehaviorSpec)
def behavior_spec(
    archetype: BehaviorArchetype,
    use_llm: bool = Query(
        False,
        description="When true, the generator will attempt to call the configured LLM runtime to compose the spec."
    ),
) -> BehaviorSpec:
    generator = BehaviorSpecGenerator()
    return generator.generate(archetype, use_llm=use_llm)



@app.post("/api/copilot/generate", response_model=CopilotGenerateResponse)
def copilot_generate(payload: CopilotGenerateRequest) -> CopilotGenerateResponse:
    file_path = payload.context.get('target_file', 'Source/Gameplay/Abilities/UAbility_Sprint.cpp')
    return CopilotGenerateResponse(
        files=[CopilotFile(path=file_path, content='// generated sprint ability code')],
        summary='Generated sprint ability module',
        diagnostics=[],
    )


@app.post("/api/copilot/explain-blueprint", response_model=CopilotExplainResponse)
def copilot_explain(payload: CopilotExplainRequest) -> CopilotExplainResponse:
    return CopilotExplainResponse(
        explanation=f"{payload.blueprint_id} routes player input to movement nodes.",
        steps=['Fetch input', 'Update movement', 'Trigger VFX'],
        related_assets=['Content/VFX/movement_fx'],
    )


@app.post("/api/copilot/asset-search", response_model=CopilotAssetSearchResponse)
def copilot_asset_search(payload: CopilotAssetSearchRequest) -> CopilotAssetSearchResponse:
    matches = [
        {'path': f"Content/VFX/{keyword}_fx", 'usage_score': '0.95'}
        for keyword in payload.keywords
    ]
    return CopilotAssetSearchResponse(matches=matches)


@app.post("/agents/npc/task", response_model=NpcTaskResponse)
def npc_task(payload: NpcTaskRequest) -> NpcTaskResponse:
    record = task_context_store.new_task()
    task_context_store.update_task(record.task_id, status=AgentStatus.running, log='NPC agent planning')
    result = plan_npc_task(payload)
    task_context_store.update_task(
        record.task_id,
        status=AgentStatus.done,
        log='NPC agent completed',
    )
    return NpcTaskResponse(
        task_id=record.task_id,
        dialogue=result['dialogue'],
        task_script=result['task_script'],
        behavior_plan=result['behavior_plan'],
    )

@app.post("/agents/feedback/{task_id}")
def submit_feedback(task_id: str, comment: str) -> dict:
    logger.info("Feedback for %s: %s", task_id, comment)
    return {"status": "received", "task_id": task_id}
