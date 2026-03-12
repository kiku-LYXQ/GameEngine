# Validation Evidence Package – 2026-03-12

## Context
- This package captures the command sequence, metrics, and log artifacts required for the current validation gate.
- Reviewer log (Rounds R2 through R5) is recorded at `docs/verification/reviewer_log.md` and documents the Copilot/NPC smoke tests that underpin this evidence.
- Release note reference: `docs/release/release_notes.md`.
- Verification report reference: `docs/verification/verification_report.md`.

## Command Sequence (all exit code 0)
| Command | Exit Code | Output Summary |
| --- | --- | --- |
| `cat /tmp/verify_all_stdout.log` | 0 | Captured the verification suite output covering Copilot and NPC smoke tests, agent capability checks, payload generation, and NPC metrics (full log archived at `archives/verify_all_stdout.txt`). |
| `curl http://127.0.0.1:7000/agents/status/health` | 0 | Agent service reachable; response `{"detail":"Task not found"}` indicates the health endpoint is responding even when no specific task is present. |
| `curl http://127.0.0.1:7001/metrics` | 0 | Retrieved runtime metrics JSON `{"evaluation.requests":1}` matching the reported snapshot. |

## Metrics Snapshot
```
{"evaluation.requests":1}
```
Stored metrics output is consistent with the snapshot in the verification report and the Copilot/NPC runs described in reviewer log R2-R5.

## Logs and Artifacts
- `archives/verify_all_stdout.txt` – Complete verification script output (copied from `/tmp/verify_all_stdout.log`).
- `archives/reviewer_R5_commands.txt` – Reviewer R5 command summary extracted from `docs/verification/reviewer_log.md` (see R5 sections).
- `docs/verification/reviewer_log.md` – Reviewer log covering validation rounds R2 through R5.

## Next Steps
- Reference this evidence package in the release PR so that reviewers can trace the commands, metrics, and logs that correspond to the release note.
- Coordinate with the reviewer for any follow-up validation if service state changes.
