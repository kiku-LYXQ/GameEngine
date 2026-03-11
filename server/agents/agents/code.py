from __future__ import annotations

from ..models import AgentTaskStep


def execute_code_agent(step: AgentTaskStep) -> str:
    return f"Generated placeholder code for {step.name}."
