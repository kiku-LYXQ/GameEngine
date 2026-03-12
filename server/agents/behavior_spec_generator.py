from __future__ import annotations

import json
import logging
import os
from typing import Callable, Optional

import httpx
from pydantic import ValidationError

from .behavior_spec import (
    BehaviorArchetype,
    BehaviorSpec,
    DEFAULT_BEHAVIOR_SPECS,
    get_default_behavior_spec,
)

logger = logging.getLogger("server.agents.behavior_spec.generator")

CompletionFn = Callable[[str], str]

_PROMPT_TEMPLATE = (
    "Produce a production-ready JSON BehaviorSpec for the {archetype} archetype. "
    "The output must be a single JSON object that exposes the following canonical fields: "
    "ObjectType, RequiredComponents, OptionalComponents, BehaviorHooks, ResourceSlots, ValidationTargets, Metadata. "
    "Describe values clearly and rely on component/asset names that can be wired into the console deployment. "
    "Example schema:\n{example}\nReturn the JSON exactly (no markdown, no surrounding text)."
)


def _build_prompt(archetype: BehaviorArchetype) -> str:
    example = json.dumps(DEFAULT_BEHAVIOR_SPECS[archetype], indent=2, ensure_ascii=False)
    return _PROMPT_TEMPLATE.format(archetype=archetype.display_name(), example=example)


def _parse_behavior_spec(payload: str) -> Optional[BehaviorSpec]:
    if not payload:
        return None

    try:
        parsed = json.loads(payload)
    except json.JSONDecodeError:
        logger.warning("BehaviorSpec parser failed to decode JSON")
        return None

    if not isinstance(parsed, dict):
        logger.warning("BehaviorSpec parser expected dict but got %s", type(parsed).__name__)
        return None

    try:
        return BehaviorSpec(**parsed)
    except ValidationError as exc:
        logger.warning("BehaviorSpec parser rejected payload: %s", exc)
        return None


class BehaviorSpecGenerator:
    def __init__(
        self,
        completion_fn: CompletionFn | None = None,
        *,
        model: Optional[str] = None,
        endpoint: Optional[str] = None,
        max_tokens: Optional[int] = None,
    ) -> None:
        self._completion_fn = completion_fn
        self._model = model or os.getenv("BEHAVIOR_SPEC_MODEL", "game-qwen-7b")
        endpoint_value = endpoint if endpoint is not None else os.getenv(
            "BEHAVIOR_SPEC_LLM_ENDPOINT", "http://127.0.0.1:7001"
        )
        self._endpoint = endpoint_value.rstrip("/") if endpoint_value else None
        max_token_value = max_tokens if max_tokens is not None else os.getenv("BEHAVIOR_SPEC_MAX_TOKENS", "256")
        try:
            self._max_tokens = int(max_token_value)
        except ValueError:
            self._max_tokens = 256

    def generate(self, archetype: BehaviorArchetype, *, use_llm: bool = False) -> BehaviorSpec:
        if not use_llm:
            return self._default_spec(archetype)

        prompt = _build_prompt(archetype)
        output: Optional[str] = None

        if self._completion_fn:
            output = self._completion_fn(prompt)
        elif self._endpoint:
            output = self._call_llm(prompt)
        else:
            logger.warning(
                "LLM BehaviorSpec requested for %s but no runtime is configured; returning default spec",
                archetype.value,
            )

        if output:
            parsed = _parse_behavior_spec(output)
            if parsed:
                return parsed
            logger.warning(
                "LLM BehaviorSpec output for %s could not be validated; returning default spec",
                archetype.value,
            )
        else:
            logger.warning(
                "LLM BehaviorSpec generator returned empty output for %s; returning default spec",
                archetype.value,
            )

        return self._default_spec(archetype)

    def _default_spec(self, archetype: BehaviorArchetype) -> BehaviorSpec:
        return get_default_behavior_spec(archetype)

    def _call_llm(self, prompt: str) -> Optional[str]:
        if not self._endpoint:
            return None

        payload = {
            "model": self._model,
            "prompt": prompt,
            "max_tokens": self._max_tokens,
            "temperature": 0.2,
        }
        url = f"{self._endpoint}/v1/completions"
        try:
            response = httpx.post(url, json=payload, timeout=5.0)
            response.raise_for_status()
        except httpx.HTTPError as exc:
            logger.warning("BehaviorSpec LLM call failed: %s", exc)
            return None

        body = response.json()
        choices = body.get("choices") or []
        for choice in choices:
            text = choice.get("text")
            if text:
                return text
        logger.warning("BehaviorSpec LLM call returned empty choices")
        return None
