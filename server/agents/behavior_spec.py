from __future__ import annotations

import json
import logging
import os
from enum import Enum
from typing import Callable, Dict, List, Optional

import httpx
from pydantic import BaseModel, ConfigDict, Field, ValidationError

logger = logging.getLogger("server.agents.behavior_spec")

CompletionFn = Callable[[str], str]


class BehaviorArchetype(str, Enum):
    character = "character"
    breakable_prop = "breakable_prop"
    interactable_prop = "interactable_prop"
    generic_actor = "generic_actor"

    def display_name(self) -> str:
        names = {
            BehaviorArchetype.character: "Character",
            BehaviorArchetype.breakable_prop: "Breakable Prop",
            BehaviorArchetype.interactable_prop: "Interactable Prop",
            BehaviorArchetype.generic_actor: "Generic Actor",
        }
        return names.get(self, self.value.title().replace("_", " "))


class BehaviorSpec(BaseModel):
    object_type: str = Field(alias="ObjectType")
    required_components: List[str] = Field(alias="RequiredComponents")
    optional_components: List[str] = Field(alias="OptionalComponents")
    behavior_hooks: Dict[str, str] = Field(alias="BehaviorHooks")
    resource_slots: List[str] = Field(alias="ResourceSlots")
    validation_targets: List[str] = Field(alias="ValidationTargets")
    metadata: Dict[str, str] = Field(alias="Metadata")

    model_config = ConfigDict(populate_by_name=True)


_DEFAULT_BEHAVIOR_SPECS: Dict[BehaviorArchetype, Dict[str, object]] = {
    BehaviorArchetype.character: {
        "ObjectType": "Character",
        "RequiredComponents": [
            "CharacterMovementComponent",
            "AIControllerComponent",
            "BlackboardComponent",
        ],
        "OptionalComponents": [
            "DialogueComponent",
            "CoverComponent",
            "StealthComponent",
        ],
        "BehaviorHooks": {
            "OnSpawn": "Initialize perception grid, equip default gear, and populate blackboard.",
            "OnDamageTaken": "Trigger hit reaction, update health display, and schedule fallback behavior.",
            "OnObjectiveUpdated": "Refresh behavior tree selectors and notify mission tracker.",
        },
        "ResourceSlots": [
            "PrimaryWeaponSlot",
            "MovementRequest",
            "DialogueContext",
            "AnimationMontageQueue",
        ],
        "ValidationTargets": [
            "BehaviorTree/CombatCore",
            "Navigation/NavMeshStreaming",
            "Animation/BlendSpace",
            "Regression/AI/CharacterState",
        ],
        "Metadata": {
            "domain": "npc",
            "pipeline_stage": "production",
            "owner": "gameplay-ai",
            "stability": "high",
        },
    },
    BehaviorArchetype.breakable_prop: {
        "ObjectType": "Prop (Breakable)",
        "RequiredComponents": [
            "DestructibleComponent",
            "DamageableComponent",
            "PhysicsConstraintComponent",
        ],
        "OptionalComponents": [
            "LootTableComponent",
            "ImpactAudioComponent",
        ],
        "BehaviorHooks": {
            "OnImpact": "Evaluate damage profile, play FX, and update durability state.",
            "OnDestroyed": "Spawn debris, notify quest tracker, and trigger cleanup timer.",
        },
        "ResourceSlots": [
            "DestructibleMesh",
            "FXSlot",
            "AudioCueSlot",
        ],
        "ValidationTargets": [
            "Physics/Destructible",
            "Audio/ImpactMix",
            "Content/Props/Breakables",
        ],
        "Metadata": {
            "domain": "props",
            "pipeline_stage": "playtest",
            "owner": "props-team",
            "stability": "medium",
        },
    },
    BehaviorArchetype.interactable_prop: {
        "ObjectType": "Prop (Interactable)",
        "RequiredComponents": [
            "InteractionComponent",
            "AnimSequenceComponent",
            "TooltipComponent",
        ],
        "OptionalComponents": [
            "QuestTriggerComponent",
            "HighlightComponent",
        ],
        "BehaviorHooks": {
            "OnInteract": "Validate user state, play interaction montage, and dispatch response.",
            "OnFocus": "Show tooltip, highlight outline, and prefetch audio cues.",
            "OnReset": "Reset cooldown timers and visual state for reuse.",
        },
        "ResourceSlots": [
            "InteractionCue",
            "VFXSlot",
            "DialogueSlot",
        ],
        "ValidationTargets": [
            "Interaction/Timing",
            "UI/Highlight",
            "Quest/FlowConsistency",
        ],
        "Metadata": {
            "domain": "props",
            "pipeline_stage": "production",
            "owner": "props-team",
            "stability": "high",
        },
    },
    BehaviorArchetype.generic_actor: {
        "ObjectType": "Generic Actor",
        "RequiredComponents": [
            "TransformComponent",
            "GenericAIController",
        ],
        "OptionalComponents": [
            "AudioEmitterComponent",
            "LightComponent",
        ],
        "BehaviorHooks": {
            "OnSpawn": "Perform registration with director, warm up state machine, and publish spawn event.",
            "OnIdle": "Cycle through idle animations and expose telemetry for instrumentation.",
            "OnScripted": "Follow timeline data and yield control to sequencer when required.",
        },
        "ResourceSlots": [
            "AnimationPlaylist",
            "AudioCueSlot",
            "SequencerBinding",
        ],
        "ValidationTargets": [
            "SpawnScript/Sequencer",
            "PathFollowing/Generic",
            "Regression/Actors/Generic",
        ],
        "Metadata": {
            "domain": "utility",
            "pipeline_stage": "production",
            "owner": "engine",
            "stability": "stable",
        },
    },
}

_PROMPT_TEMPLATE = (
    "Produce a production-ready JSON BehaviorSpec for the {archetype} archetype. "
    "The output must be a single JSON object that exposes the following canonical fields: "
    "ObjectType, RequiredComponents, OptionalComponents, BehaviorHooks, ResourceSlots, ValidationTargets, Metadata. "
    "Describe values clearly and rely on component/asset names that can be wired into the console deployment. "
    "Example schema:\n{example}\nReturn the JSON exactly (no markdown, no surrounding text)."
)


def _build_prompt(archetype: BehaviorArchetype) -> str:
    example = json.dumps(_DEFAULT_BEHAVIOR_SPECS[archetype], indent=2, ensure_ascii=False)
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
    except ValidationError:
        logger.warning("BehaviorSpec parser rejected payload")
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
        endpoint_value = endpoint if endpoint is not None else os.getenv("BEHAVIOR_SPEC_LLM_ENDPOINT", "http://127.0.0.1:7001")
        self._endpoint = endpoint_value.rstrip("/") if endpoint_value else None
        max_token_value = max_tokens if max_tokens is not None else os.getenv("BEHAVIOR_SPEC_MAX_TOKENS", "256")
        try:
            self._max_tokens = int(max_token_value)
        except ValueError:
            self._max_tokens = 256

    def generate(self, archetype: BehaviorArchetype, *, use_llm: bool = False) -> BehaviorSpec:
        prompt = _build_prompt(archetype)
        if not use_llm and not self._completion_fn:
            return self._default_spec(archetype)

        output: Optional[str] = None
        if use_llm:
            if self._completion_fn:
                output = self._completion_fn(prompt)
            elif self._endpoint:
                output = self._call_llm(prompt)
            else:
                logger.warning(
                    "LLM BehaviorSpec requested for %s but no runtime is configured; returning default spec",
                    archetype.value,
                )
        else:
            return self._default_spec(archetype)

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
        data = _DEFAULT_BEHAVIOR_SPECS[archetype]
        return BehaviorSpec(**data)

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
