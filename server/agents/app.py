from __future__ import annotations

import logging
from typing import Dict
from uuid import uuid4

from fastapi import FastAPI, BackgroundTasks, HTTPException, Query

from .agents.asset import find_assets
from .agents.behavior_spec import BehaviorArchetype, BehaviorSpec
from .behavior_spec_generator import BehaviorSpecGenerator
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
    CopilotBindingPlan,
    CopilotManifestHint,
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


@app.get("/agents/status/health")
def service_health() -> dict[str, str]:
    return {"detail": "Task queue healthy", "status": "ok"}


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



def _resolve_behavior_archetype(context: Dict[str, str]) -> BehaviorArchetype:
    raw = context.get("archetype")
    if raw:
        try:
            return BehaviorArchetype(raw.lower())
        except ValueError:
            logger.warning("Unknown archetype '%s'; falling back to Character", raw)
    return BehaviorArchetype.character


def _build_binding_plan(spec: BehaviorSpec, spec_id: str) -> CopilotBindingPlan:
    description = f"Blueprint binding plan for {spec.object_type} ({spec_id})"
    steps = [
        f"Create a Blueprint named BP_{spec_id} inheriting from the generated class.",
        f"Wire Execute<Hook>Hook helpers for each behavior hook: {', '.join(spec.behavior_hooks.keys())}",
        f"Assign resource slots: {', '.join(spec.resource_slots)} with valid assets before runtime.",
        "Use the manifest to confirm reserved slots and validation targets.",
    ]
    return CopilotBindingPlan(description=description, steps=steps)


def _build_manifest_hint(spec: BehaviorSpec, spec_id: str) -> CopilotManifestHint:
    return CopilotManifestHint(
        spec_id=spec_id,
        resource_slots=spec.resource_slots,
        behavior_hooks=list(spec.behavior_hooks.keys()),
    )


@app.post("/api/copilot/generate", response_model=CopilotGenerateResponse)
def copilot_generate(payload: CopilotGenerateRequest) -> CopilotGenerateResponse:
    archetype = _resolve_behavior_archetype(payload.context)
    generator = BehaviorSpecGenerator()
    logger.info("Copilot generate request use_llm=%s archetype=%s", payload.use_llm, archetype.value)
    spec = generator.generate(archetype, use_llm=payload.use_llm)
    spec_id = payload.context.get("spec_id") or f"{archetype.value}-{uuid4().hex[:6]}"
    binding_plan = _build_binding_plan(spec, spec_id)
    manifest_hint = _build_manifest_hint(spec, spec_id)
    file_path = payload.context.get("target_file", "Source/Gameplay/Abilities/UAbility_Sprint.cpp")
    summary = payload.context.get("summary", "Generated sprint ability module")
    return CopilotGenerateResponse(
        files=[CopilotFile(path=file_path, content="// generated sprint ability code")],
        summary=summary,
        behavior_spec=spec,
        binding_plan=binding_plan,
        manifest_hint=manifest_hint,
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


@app.get("/health/metrics")
def health_metrics() -> dict[str, object]:
    return {
        "agent": {"status": "ok", "queue_depth": 0},
        "llm": {"status": "ok", "availability": "ok"},
    }
