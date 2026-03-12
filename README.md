# GameEngine Workspace Layout

This workspace organizes AI/Unreal artifacts, verification evidence, and supporting services to keep generated outputs and logs easy to find.

## Top-level directories
- `generated/behavior_templates` – canonical BehaviorTemplateGenerator output (skeleton `.h`/`.cpp`, binding plans, manifest).
- `archives/behavior_validation` – archival evidence produced by `scripts/verify_behavior.sh` (`generated_code/`, `verification_logs/`, `runtime_metrics/`, `release_notes/`). Treat this as the single source of truth for reviewer artifacts.
- `logs/verification` – symlink pointing to `archives/behavior_validation/verification_logs` so verification builds are accessible from the logs folder as well.
- `docs/` – design and verification guidance (`system_design/behavior_generation_framework.md`, `verification/behavior_validation.md`, etc.).
- `scripts/` – automation (`verify_behavior.sh`, `verify_all.sh`, validation helpers). The verification pipeline writes outputs into `archives/behavior_validation` which in turn is aliased from `logs/verification`.
- `server/` – FastAPI services (`agents`, `llm_runtime`, `rag`) that drive Copilot and BehaviorSpec APIs.
- `ue_plugin/AI_Copilot` – Unreal plugin sources that consume BehaviorSpec and emit template artifacts.

## Notable files
- `behavior_spec.json` / `behavior_spec_example.json` – sample archetype specs referenced by the generator and verification script.
- `logs/verification_build.log` – high-level verification run log.

## Best practices
1. Generate artifacts via `scripts/verify_behavior.sh behavior_spec.json`, then archive output under `archives/behavior_validation` for reviewer evidence. The script already logs into the symlinked `logs/verification` and saves release notes plus runtime metrics.
2. Review template results in `generated/behavior_templates` (or `GeneratedBehaviorTemplates` for legacy paths) to find skeleton `.h/.cpp`, binding plans, and manifest metadata.
3. Consult `docs/system_design/behavior_generation_framework.md` for architecture context and `docs/verification/behavior_validation.md` for how evidence directories map to tests.
