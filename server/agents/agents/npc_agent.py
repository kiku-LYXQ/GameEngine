from __future__ import annotations

from typing import Dict, List

from ..models import NpcTaskRequest


def plan_npc_task(payload: NpcTaskRequest) -> Dict[str, object]:
    # Placeholder logic for NPC behavior planning
    dialogue: List[str] = [
        f"{payload.npc_id} says: I will {payload.intent} for {payload.behavior}.",
        "NPC: Engage behavior tree node Explore!",
    ]
    task_script = f"Quest for {payload.npc_id}: follow behavior {payload.behavior} with intent {payload.intent}."
    behavior_plan = {
        "MoveTo": "TargetPosition",
        "PlayAnimation": "NPCIdle",
        "TriggerDialog": payload.intent,
    }
    if payload.chunk_id:
        behavior_plan["ChunkContext"] = payload.chunk_id
    return {
        "dialogue": dialogue,
        "task_script": task_script,
        "behavior_plan": behavior_plan,
    }
