from __future__ import annotations

from ..models import AgentTaskStep


def create_documentation(step: AgentTaskStep) -> str:
    return f"Documentation stub for {step.name}."
