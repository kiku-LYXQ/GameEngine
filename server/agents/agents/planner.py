from __future__ import annotations

from typing import List

from ..models import AgentTaskStep, CostEstimate


def plan_from_prompt(prompt: str) -> List[AgentTaskStep]:
    """拆解用户需求，返回任务步骤列表，并附带 cost estimate。"""
    steps = [
        AgentTaskStep(
            name="analysis",
            description="Analyze user intent and gather context.",
            cost_estimate=CostEstimate(tokens=80, latency_ms=200),
        ),
        AgentTaskStep(
            name="code",
            description="Generate C++/Blueprint based on analysis.",
            cost_estimate=CostEstimate(tokens=180, latency_ms=400),
        ),
        AgentTaskStep(
            name="assets",
            description="Collect or recommend assets.",
            cost_estimate=CostEstimate(tokens=120, latency_ms=250),
        ),
        AgentTaskStep(
            name="docs",
            description="Summarize design details.",
            cost_estimate=CostEstimate(tokens=90, latency_ms=220),
        ),
    ]
    return steps
