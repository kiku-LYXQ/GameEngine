from __future__ import annotations

import json

from server.agents.behavior_spec import BehaviorArchetype
from server.agents.behavior_spec_generator import BehaviorSpecGenerator


def test_default_spec_includes_all_schema_fields() -> None:
    generator = BehaviorSpecGenerator()

    for archetype in BehaviorArchetype:
        spec = generator.generate(archetype)
        payload = spec.model_dump(by_alias=True)

        assert payload["ObjectType"], "ObjectType must be populated"
        assert payload["RequiredComponents"], "RequiredComponents must be populated"
        assert payload["OptionalComponents"] is not None
        assert payload["BehaviorHooks"], "BehaviorHooks must be populated"
        assert payload["ResourceSlots"], "ResourceSlots must be populated"
        assert payload["ValidationTargets"], "ValidationTargets must be populated"
        assert payload["Metadata"], "Metadata must be populated"


def test_llm_completion_parse_override() -> None:
    def stub_completion(prompt: str) -> str:
        assert "BehaviorSpec" in prompt
        return json.dumps({
            "ObjectType": "Custom Test Actor",
            "RequiredComponents": ["ComponentA"],
            "OptionalComponents": ["ComponentB"],
            "BehaviorHooks": {"OnTest": "Mirror prompt"},
            "ResourceSlots": ["SlotA"],
            "ValidationTargets": ["TargetA"],
            "Metadata": {"source": "stub"},
        })

    generator = BehaviorSpecGenerator(completion_fn=stub_completion)
    spec = generator.generate(BehaviorArchetype.generic_actor, use_llm=True)

    assert spec.object_type == "Custom Test Actor"
    assert spec.required_components == ["ComponentA"]
    assert spec.metadata["source"] == "stub"


def test_invalid_completion_falls_back_to_default() -> None:
    generator = BehaviorSpecGenerator(completion_fn=lambda prompt: "not json")
    spec = generator.generate(BehaviorArchetype.character, use_llm=True)

    assert spec.object_type == "Character"


def test_llm_disabled_returns_default_without_executing_runtime() -> None:
    called = []

    def stub(prompt: str) -> str:
        called.append(True)
        return json.dumps({
            "ObjectType": "Instrumented Actor",
            "RequiredComponents": ["InstrumentComponent"],
            "OptionalComponents": [],
            "BehaviorHooks": {"OnTest": "tick"},
            "ResourceSlots": ["SlotX"],
            "ValidationTargets": ["TargetX"],
            "Metadata": {"source": "stub"},
        })

    generator = BehaviorSpecGenerator(completion_fn=stub)
    spec = generator.generate(BehaviorArchetype.character, use_llm=False)

    assert not called, "LLM runtime should not run when use_llm is false"
    assert spec.object_type == "Character"
