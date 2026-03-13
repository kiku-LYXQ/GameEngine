# Behavior Generation Framework

## BehaviorSpec Schema

The BehaviorSpec archetype schema codifies how runtime dashboards, production behavior trees, and verification tooling agree on the actor metadata that every downstream system consumes. Every BehaviorSpec payload exposes the following canonical fields:

| Field | Description |
| --- | --- |
| **ObjectType** | Human-readable archetype identifier (Character, Prop (Breakable), Prop (Interactable), Generic Actor, etc.). |
| **RequiredComponents** | Components that must exist for the archetype to function (movement, AI controllers, interaction drivers, etc.). |
| **OptionalComponents** | Extras such as dialogue/cover helpers, audio emitters, or telemetry that are nice to have but not mandatory. |
| **BehaviorHooks** | Named lifecycle callbacks (OnSpawn, OnInteract, OnDestroyed, etc.) paired with the narrative/logic intent executed at each hook. |
| **ResourceSlots** | Slots reserved for assets or contextual data (primary weapon, animation montages, audio cues, sequencer bindings, etc.). |
| **ValidationTargets** | Asset/test suites gated on this archetype (behavior trees, regression suites, navigation bundles, audio mixes). |
| **Metadata** | Free-form key/value pairs (domain, owner, pipeline stage, stability, etc.) used for routing, alerting, and ownership tracking.

The schema is implemented via `BehaviorSpec` (Pydantic) inside `server/agents/behavior_spec.py`. That module also defines `BehaviorArchetype` and the curated payloads that back every archetype. `BehaviorSpecGenerator` lives next door in `server/agents/behavior_spec_generator.py`; it builds the canonical prompt, validates JSON, and exposes `GET /agents/behavior-spec/{archetype}` so every agent or tool can fetch production-ready JSON that wires straight into the behavior pipeline.

## Archetype Defaults & Example Manifest

The generator ships with curated, production-ready defaults for the archetypes the pipeline depends on. Each default is included in `behavior_spec_example.json`, which mirrors the payload structure returned by the HTTP endpoint. The file can be used as a quick reference or sample artifact for internal docs and verification evidence.

- **Character** – Includes locomotion, AI controller, and blackboard components. Hooks cover `OnSpawn`, `OnDamageTaken`, and `OnObjectiveUpdated`, while resource slots cover weapon contexts, animation queues, and dialogue buffers. Validation targets point at combat behavior trees, navigation streaming, animation blend spaces, and regression suites. Metadata flags the pipeline stage, owner, and stability level.
- **Breakable Prop** – Involves destructible, damageable, and physics constraint components. Hooks such as `OnImpact` and `OnDestroyed` coordinate FX, loot, and cleanup. Resource slots include destructible meshes, VFX, and audio cues; validation targets span destructible physics and impact audio. Metadata notes the props domain and `playtest` readiness.
- **Interactable Prop** – Drives interaction/tooltip components with optional quest triggers and highlight helpers. Hooks include `OnInteract`, `OnFocus`, and `OnReset`. Resource slots provide interaction cue references, VFX, and dialogue transitions; validation touches interaction timing, UI highlighting, and quest flow consistency. Metadata retains props ownership with a production readiness flag.
- **Generic Actor** – Provides a minimal actor (transform + generic controller) with optional audio/light helpers. Hooks such as `OnSpawn`, `OnIdle`, and `OnScripted` support sequencer alignment. Resource slots include animation playlists, audio cues, and sequencer bindings; validation spans spawn scripts, path-following, and regression coverage. Metadata marks the utility domain and steady stability rating.

The `behavior_spec_example.json` artifact mirrors these defaults and is stored at the repository root so it can be zipped into verification submissions or referenced by other systems without running the service.

## Prompt Template & LLM Runtime Integration

`BehaviorSpecGenerator` can operate in two modes:

1. **Fallback mode (default)** – When the orchestrator calls the endpoint with `?use_llm=false` (or when no runtime is configured) the generator immediately returns the curated default for the requested archetype without calling an LLM.
2. **LLM-assisted mode** – When `use_llm=true` and an endpoint is configured, the generator builds a prompt that explicitly lists every required schema field and shows the curated JSON example. The internal `_PROMPT_TEMPLATE` reads:

```
Produce a production-ready JSON BehaviorSpec for the {archetype} archetype. The output must be a single JSON object that exposes the following canonical fields: ObjectType, RequiredComponents, OptionalComponents, BehaviorHooks, ResourceSlots, ValidationTargets, Metadata. Describe values clearly and rely on component/asset names that can be wired into the console deployment. Example schema:
{example}
Return the JSON exactly (no markdown, no surrounding text).
```

The prompt is posted to `http://127.0.0.1:7001/v1/completions` by default, using `game-qwen-7b` (configurable via `BEHAVIOR_SPEC_MODEL`) and a token budget controlled by `BEHAVIOR_SPEC_MAX_TOKENS`. The endpoint URL itself is overridable through `BEHAVIOR_SPEC_LLM_ENDPOINT`. If the runtime returns valid JSON, it is validated by Pydantic before being returned. Any invalid output—malformed JSON, missing fields, or parsing/validation errors—triggers warning logs (`BehaviorSpec generator returned empty output...`, `could not be validated...`, etc.) and falls back to the curated default. This ensures no downstream caller ever receives a partial schema.

## API Surface & Consumption

To fetch a BehaviorSpec:

```
curl http://127.0.0.1:7000/agents/behavior-spec/character?use_llm=false
```

Add `?use_llm=true` when an LLM runtime is available and you specifically want an AI-generated variation. Every response is guaranteed to provision `ObjectType`, `RequiredComponents`, `OptionalComponents`, `BehaviorHooks`, `ResourceSlots`, `ValidationTargets`, and `Metadata` because the data is validated through Pydantic prior to serialization.

Developers can also point tooling at `behavior_spec_example.json` when they need a persisted version of the canonical payloads without hitting the server.

## Copilot Template Generation & Blueprint Binding

`AI_Copilot` now drives a `BehaviorTemplateGenerator` inside the plugin. When a BehaviorSpec payload is attached to a Copilot response, the generator writes a `[Class].h`/`.cpp` skeleton, a `_BindingPlan.txt`, and a `_Manifest.txt` into `generated/behavior_templates/`. The binding plan documents the Blueprint inheritance strategy, logs the BehaviorSpec ID, and walks the Blueprint developer through wiring the generated `Execute<Hook>Hook` helpers together with the resource slots listed below. The manifest mirrors those paths and summarizes the resource reservations so reviewers can trace every artifact back to the spec.

#### BehaviorSpec-driven CoPilot contract

The Copilot backend now returns a behavior-spec-first payload for `/api/copilot/generate`. The default response includes:

```json
{
  "summary": "Generated sprint ability module",
  "behavior_spec": { /* canonical BehaviorSpec schema & metadata */ },
  "binding_plan": {
    "description": "Blueprint binding plan for Character",
    "steps": [
      "Create Blueprint BP_{spec_id} inheriting from the generated actor",
      "Wire the Execute<Hook>Hook helpers for every BehaviorHook",
      "Reserve the resource slots listed in the manifest before runtime",
      "Confirm validation targets align with the manifest hints"
    ]
  },
  "manifest_hint": {
    "spec_id": "character-ab12",
    "resource_slots": ["PrimaryWeaponSlot", "MovementRequest", "AnimationMontageQueue"],
    "behavior_hooks": ["OnSpawn", "OnDamageTaken", "OnObjectiveUpdated"]
  },
  "files": [ /* optional stub files for backwards compatibility */ ]
}
```

`behavior_spec` is now mandatory; the panel uses it to parse archetype, ResourceSlots, and BehaviorHooks before invoking `BehaviorTemplateGenerator::GenerateSkeleton`. The `binding_plan` and `manifest_hint` entries act as UI-friendly hints while the actual skeleton/manifest writing still happens inside the plugin.

### Blueprint Binding Hints & Reserved Slots

The resource slots captured in the generated header remain reserved for Blueprint authors and show up as `UPROPERTY` hints in the skeleton, the binding plan, and the manifest:

- **StaticMeshSlot (`UStaticMesh*`)** – bind the primary static mesh asset before any hooks run so the actor carries the right silhouette.
- **SkeletalMeshSlot (`USkeletalMesh*`)** – the skeletal mesh that the animation hooks reference for retargeting and attachment.
- **MontageSlot (`UAnimMontage*`)** – the montage slot the hooking logic triggers via `Execute<Hook>Hook` before playing actions such as attacks or hit reactions.
- **NiagaraSlot (`UNiagaraSystem*`)** – reserved for ribbon trails, impact FX, or charged-ability motifs keyed to BehaviorHooks.
- **SoundCueSlot (`USoundCue*`)** – binds the auditory feedback (damage shout, ability cast, etc.) referenced by the hook descriptions.
- **MaterialSlot (`UMaterialInterface*`)** – overrides the material while the hooks coordinate decals, emissives, or blend transitions.
- **BlueprintClassSlot (`TSubclassOf<AActor>`)** – reserved for injecting optional Blueprint/actor subclasses, letting the manifest cite `OptionalBlueprintClass` when a variant or decorator is desired.

Each BehaviorHook section in the plan repeats how to call `Execute<Hook>Hook`, which Blueprint event to map, and which of the slots above should be set before or during that hook’s execution.

### Verification via Copilot Panel

Trigger the template pipeline the same way the Panel does:

```
curl http://127.0.0.1:7000/api/copilot/generate
```

The API response and CopilotPanel logs now mention the BehaviorSpec ID, list the generated skeleton/binding plan/manifest paths, and record those files under `generated/behavior_templates/`. Confirm the HTTP stack is healthy before running the command (the manifest references both `curl http://127.0.0.1:7000/api/copilot/generate` and `curl http://127.0.0.1:7000/agents/status/health`). Reviewers can open the manifest to see the reserved slots, the BehaviorHooks summary, and the verification commands without touching the source generator again.

### Copilot Panel (UI) Experience & Significance

Within the UE editor, the Copilot Panel exposes this workflow behind a single `Generate Behavior Template` button. The panel automatically detects the currently selected BehaviorSpec (or Blueprint) and, when clicked, sends the same `curl` request above to emit the skeleton/binding plan/manifest into `generated/behavior_templates/`. The manifest path is surfaced directly in the panel so artists can open it instantly.

Health indicators are also presented in the panel by polling `/agents/status/health`, `/agents/capabilities`, and `/health/metrics`. If any endpoint is unreachable, the generate button grays out and the UI prompts the user to rerun `scripts/verify_behavior.sh` to refresh the health data. After generation, resource slots (StaticMesh/SkeletalMesh/Montage/Niagara/SoundCue/Material/BlueprintClass) and BehaviorHooks are highlighted with usage tips, letting Blueprint authors map `Execute<Hook>Hook` events without staring at the raw `.txt` files.

**Significance for UE newcomers**: The Copilot Panel removes the need to memorize curl commands or dig through directories. It records the BehaviorSpec ID, the generated paths, and the CLI health checks in one place, so reviewers, artists, or designers can trace every artifact back to the spec with no extra effort. Use the panel as the primary entry point—open the manifest/plan from the UI and follow the tips to wire the generated code into your Blueprint.

## Testing & Verification

### Verification Commands

- `pytest server/agents/tests/test_behavior_spec.py` — ensures every archetype yields payloads that contain the canonical schema fields and that invalid LLM overrides fall back to defaults.
- `curl http://127.0.0.1:7000/agents/behavior-spec/character?use_llm=false` — exercises the live endpoint with LLM disabled and produces a JSON artifact that can be captured for verification.

Capture the outputs of these commands under `archives/behavior_validation/verification_logs/` (e.g., `behavior_spec_pytest.txt` and `behavior_spec_api.json`) and include them in the verification bundle.

### Evidence Directories

`archives/behavior_validation/` contains the evidence that reviewers expect to see:

- `generated_code/` – Script-produced artifacts such as `behavior_plan.json` and `behavior_blueprint.py` stubs that summarize how the BehaviorSpec maps to C++ scaffolds.
- `verification_logs/` – Aggregated logs (e.g., `behavior_validation.log`, `code_validity.log`, `behavior_analysis.log`) plus the outputs from the BehaviorSpec-specific commands (`behavior_spec_pytest.txt`, `behavior_spec_api.json`).
- `runtime_metrics/` – JSON snapshots (`llm_metrics.json`, `agent_health_metrics.json`) that track latency, success rates, and resource usage for the BehaviorSpec runtime.
- `release_notes/` – `behavior_validation_note.md` summarizes the behavior IDs, binding counts, resource table, CLI results, and pointers to the artifacts above so reviewers can trace every claim.

The existing `scripts/verify_behavior.sh` orchestrates these steps, writing each artifact into the directories above while also echoing a JSON summary to stdout. Re-run it after schema/API changes to refresh every evidence bucket.

### Offline Sample Reference

`behavior_spec_example.json` mirrors the HTTP payloads and serves as a lightweight artifact you can bundle when hitting the live service is not practical. Keep it in sync with `DEFAULT_BEHAVIOR_SPECS` and refer to it in test rundowns or release notes when describing how the schema is wired.
