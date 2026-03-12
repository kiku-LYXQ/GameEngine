# CI Badge Snippets

## Copilot validation smoke test
```
[![Copilot verification](https://img.shields.io/badge/verification-verify_all%20pass-brightgreen)](https://github.com/your-org/GameEngine/actions/workflows/verify.yml)
```
- Points at `scripts/verify_all.sh` (see `docs/system_design/interaction_flow.md` for the verification flow). Keep the badge green by running `verify_all.sh` before pushing.

## LLM runtime metrics
```
[![LLM metrics](https://img.shields.io/badge/metrics-evaluation.requests%3D1-brightgreen)](http://127.0.0.1:7001/metrics)
```
- Reflects the `/llm_runtime/metrics` snapshot captured in `docs/verification/reviewer_log.md` (evaluation.requests = 1 per verify_all round). Update the badge text whenever the snapshot changes.
