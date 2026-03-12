from __future__ import annotations

from enum import Enum
from typing import Dict, List

from pydantic import BaseModel, ConfigDict, Field


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


DEFAULT_BEHAVIOR_SPECS: Dict[BehaviorArchetype, Dict[str, object]] = {
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


def get_default_behavior_spec(archetype: BehaviorArchetype) -> BehaviorSpec:
    """Return the curated BehaviorSpec payload that backs the prompt example."""
    data = DEFAULT_BEHAVIOR_SPECS[archetype]
    return BehaviorSpec(**data)
