# AI Copilot Validation Release Note

## AppStyle module refactor
- `FAI_CopilotModule` and `FAICopilotCommands` now consistently draw on `Styling/AppStyle.h` to register the Copilot tab, toolbar button, and menu entry, ensuring the panel shares `LevelEditor.GameSettings` iconography and workspace menu structure from `ue_plugin/AI_Copilot/Source/AI_Copilot/Private/AI_Copilot.cpp`/`.h`.
- The refactor clarifies the link between the module lifecycle, command bindings, and the `WorkspaceMenuStructure` integration, making the Copilot workspace feel native to the Level Editor.

## Copilot Panel stability improvements
- `SCopilotPanel` now drives explicit status updates for every network interaction: capability fetch logs a success or fallback default record, metrics requests surface `Metrics loading...`, `Metrics unavailable`, or `Metrics parse error`, and NPC sample requests log success/failure and refresh the behavior plan view without stalling the UI (see `ue_plugin/AI_Copilot/Source/AI_Copilot/Private/CopilotPanel.cpp`).
- Copilot prompt handling appends contextual chunk selection logs, updates the summary/file widgets on the GameThread, and keeps execution logs and capability counts in sync so the panel clearly communicates its current state to the designer.

## Metrics snapshot (via `scripts/verify_all.sh`)
- `evaluation.requests`: `1` per round (see `docs/verification/reviewer_log.md` entries for R2–R5).
- The smoke-test pipeline also reports `/llm_runtime/metrics` counters described in `docs/system_design/interaction_flow.md`, so this badge can surface real-time telemetry after `verify_all` runs.

## Verification command
- Run `markdownlint docs/release/*.md` to catch formatting issues before publishing these notes.

## References
- `docs/system_design/interaction_flow.md`
- `docs/verification/reviewer_log.md`
