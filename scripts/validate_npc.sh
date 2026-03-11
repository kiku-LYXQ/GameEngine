#!/usr/bin/env bash
set -euo pipefail

AGENT_URL="http://127.0.0.1:7000"
LLM_URL="http://127.0.0.1:7001"

printf "1. Run NPC task\n"
NPC_ID="npc_01"
TASK_ID=$(curl -sSL -X POST "$AGENT_URL/agents/npc/task" -H 'Content-Type: application/json' \
  -d '{"npc_id": "npc_01", "behavior": "patrol", "intent": "guard the gate", "chunk_id": "chunk-001"}' | jq -r '.task_id')

printf "2. Fetch NPC logs\n"
curl -sSL "$AGENT_URL/agents/logs/$TASK_ID" | jq

printf "3. Evaluate NPC dialogue model\n"
curl -sSL -X POST "$LLM_URL/models/npc-dialogue-7b/evaluate" -H 'Content-Type: application/json' \
  -d '{"prompt": "NPC dialogue test", "expected": "friendly guard"}' | jq

printf "4. Confirm LLM metrics\n"
curl -sSL "$LLM_URL/metrics" | jq

printf "NPC validation complete.\n"
