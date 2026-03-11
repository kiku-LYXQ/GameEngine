from __future__ import annotations

from typing import List

from ..models import AgentTaskStep


def plan_from_prompt(prompt: str) -> List[AgentTaskStep]:
    """拆解用户需求，返回任务步骤列表。"""
    steps = [
        AgentTaskStep(name="analysis", description="Analyze user intent and gather context."),
        AgentTaskStep(name="code", description="Generate C++/Blueprint based on analysis."),
        AgentTaskStep(name="assets", description="Collect or recommend assets."),
        AgentTaskStep(name="docs", description="Summarize design details."),
    ]
    return steps
