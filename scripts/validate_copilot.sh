#!/usr/bin/env bash
set -euo pipefail

AGENT_URL="http://127.0.0.1:7000"
LLM_URL="http://127.0.0.1:7001"

printf "1. Check agent capabilities\n"
curl -sSL "$AGENT_URL/agents/capabilities" | jq

printf "2. Fire simple agent task\n"
TASK_ID=$(curl -sSL -X POST "$AGENT_URL/agents/task" -H 'Content-Type: application/json' \
  -d '{"prompt": "Create sprint ability", "project_context": {"module": "Gameplay"}}' | jq -r '.task_id')

printf "3. Poll task status\n"
for i in {1..5}; do
  STATUS=$(curl -sSL "$AGENT_URL/agents/status/$TASK_ID" | jq -r '.status')
  echo "Attempt $i status=$STATUS"
  if [[ "$STATUS" == "done" ]]; then
    break
  fi
  sleep 1
done

printf "4. Fetch task logs\n"
curl -sSL "$AGENT_URL/agents/logs/$TASK_ID" | jq

printf "5. Trigger Copilot payload\n"
curl -sSL -X POST "$AGENT_URL/api/copilot/generate" -H 'Content-Type: application/json' \
  -d '{"prompt": "Explain chunk", "context": {"chunk_id": "chunk-001"}}' | jq

printf "6. Check LLM runtime metrics\n"
curl -sSL "$LLM_URL/metrics" | jq

printf "Validation scripts completed."
