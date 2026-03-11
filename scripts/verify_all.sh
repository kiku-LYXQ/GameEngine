#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
AGENT_URL="http://127.0.0.1:7000"
LLM_URL="http://127.0.0.1:7001"

printf "Starting verification suite...\n"
printf "1. Ensure services are running (agent + llm).\n"
curl -sSL "$AGENT_URL/agents/status/health" >/dev/null || true
curl -sSL "$LLM_URL/metrics" >/dev/null || true

printf "2. Copilot smoke test\n"
bash "$ROOT/validate_copilot.sh"

printf "3. NPC smoke test\n"
bash "$ROOT/validate_npc.sh"

printf "4. Metrics snapshot\n"
curl -sSL "$LLM_URL/metrics"

printf "Validation suite completed.\n"
