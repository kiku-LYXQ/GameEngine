# Release Notes – GameEngine Validation Release (2026-03-12)

## Summary
- Captured the latest validation evidence package for the GameEngine integration perimeter.
- Confirmed Copilot and NPC services operate on ports 7000/7001 with the previously reported service contracts.
- Locked the release on the documented verification snapshot and archived command logs for traceability.

## Highlights
- Services `server.agents.app` (port 7000) and `server.llm_runtime.app` (port 7001) remain responsive under the smoke test suite.
- NPC dialogue validation maintains stability with the `npc-dialogue-7b` model and steady metrics (`evaluation.requests` remains at 1).
- Verification artifacts (logs, metrics dumps, command outputs) are stored in `docs/verification` and `archives` per validation policy.

## Next Steps
- Sync this release note to the main release tracker once the evidence package and reviewer notes complete the PR.
- Ensure the verification report and evidence package stay coordinated for downstream auditing.
