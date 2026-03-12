#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$ROOT/.." && pwd)"
AGENT_URL="http://127.0.0.1:7000"
LLM_URL="http://127.0.0.1:7001"

if [[ $# -ne 1 ]]; then
  printf "Usage: %s <behavior_spec.json>\n" "$(basename "$0")" >&2
  exit 1
fi

SPEC_INPUT="$1"
SPEC_PATH="$(cd "$PWD" && realpath "$SPEC_INPUT")"
if [[ ! -f "$SPEC_PATH" ]]; then
  printf "Behavior spec file not found: %s\n" "$SPEC_INPUT" >&2
  exit 1
fi

EVIDENCE_ROOT="$REPO_ROOT/archives/behavior_validation"
GENERATED_DIR="$EVIDENCE_ROOT/generated_code"
LOG_DIR="$EVIDENCE_ROOT/verification_logs"
METRICS_DIR="$EVIDENCE_ROOT/runtime_metrics"
RELEASE_DIR="$EVIDENCE_ROOT/release_notes"

for dir in "$GENERATED_DIR" "$LOG_DIR" "$METRICS_DIR" "$RELEASE_DIR"; do
  mkdir -p "$dir"
done

MAIN_LOG="$LOG_DIR/behavior_validation.log"
: > "$MAIN_LOG"

LOG_PREFIX="[behavior-verify]"
log() {
  printf "%s %s\n" "$LOG_PREFIX" "$*" | tee -a "$MAIN_LOG"
}

log "Starting behavior verification pipeline"
log "Spec provided: $SPEC_PATH"

SUMMARY_PATH="$GENERATED_DIR/behavior_plan.json"
BLUEPRINT_PATH="$GENERATED_DIR/behavior_blueprint.py"

log "Step 1/5: 语法 & 代码有效性检验"
CODE_VALIDITY_LOG="$LOG_DIR/code_validity.log"
: > "$CODE_VALIDITY_LOG"
PYTHONDONTWRITEBYTECODE=1 python3 -m compileall -q "$REPO_ROOT/server/agents" "$REPO_ROOT/server/llm_runtime" > "$CODE_VALIDITY_LOG" 2>&1
log "Code validity check finished (details in $CODE_VALIDITY_LOG)"
cat "$CODE_VALIDITY_LOG" >> "$MAIN_LOG"

log "Step 2/5: 行为规范、资源与绑定一致性分析"
BEHAVIOR_LOG="$LOG_DIR/behavior_analysis.log"
: > "$BEHAVIOR_LOG"
python3 <<PY > "$BEHAVIOR_LOG"
import json
import pathlib

spec_path = pathlib.Path("$SPEC_PATH")
summary_path = pathlib.Path("$SUMMARY_PATH")
blueprint_path = pathlib.Path("$BLUEPRINT_PATH")

if not spec_path.exists():
    raise SystemExit(f"Spec file not found: {spec_path}")

spec = json.loads(spec_path.read_text())
behavior_section = spec.get("behavior") or spec.get("behaviors")
if behavior_section is None:
    raise SystemExit("Spec must define a 'behavior' or 'behaviors' section")

if isinstance(behavior_section, list):
    if len(behavior_section) != 1:
        raise SystemExit("Expected a single behavior definition, found multiple entries")
    behavior_section = behavior_section[0]

steps = behavior_section.get("steps")
if not isinstance(steps, list) or not steps:
    raise SystemExit("Behavior definition must include a non-empty 'steps' list")

invalid_steps = [step for step in steps if not isinstance(step, dict) or "name" not in step or "type" not in step]
if invalid_steps:
    raise SystemExit("Each behavior step needs to be a dict with 'name' and 'type'")

resources = spec.get("resources")
if not isinstance(resources, list) or not resources:
    raise SystemExit("Spec must declare a non-empty 'resources' list")

invalid_resources = [res for res in resources if not isinstance(res, dict) or any(key not in res for key in ("name", "type", "path"))]
if invalid_resources:
    raise SystemExit("Every resource entry must define 'name', 'type', and 'path'")

bindings = spec.get("bindings")
if not isinstance(bindings, list) or not bindings:
    raise SystemExit("Spec must provide at least one binding between behaviors and resources")

resource_names = {res["name"] for res in resources if isinstance(res, dict) and "name" in res}
invalid_bindings = [binding for binding in bindings if not isinstance(binding, dict) or "resource" not in binding or "behavior" not in binding or binding.get("resource") not in resource_names]
if invalid_bindings:
    raise SystemExit("Each binding must map an existing resource to a behavior")

behavior_id = behavior_section.get("id") or behavior_section.get("name") or "behavior-unnamed"
behavior_name = behavior_section.get("name") or behavior_section.get("title") or behavior_id

step_overview = [
    {
        "name": step["name"],
        "type": step["type"],
        "target": step.get("target") or step.get("phase"),
    }
    for step in steps[:5]
]

resource_overview = [
    {
        "name": res["name"],
        "type": res["type"],
        "path": res["path"],
        "tags": res.get("tags"),
    }
    for res in resources[:5]
]

binding_overview = [
    {
        "behavior": binding["behavior"],
        "resource": binding["resource"],
        "mode": binding.get("mode"),
    }
    for binding in bindings[:5]
]

summary = {
    "spec": spec_path.name,
    "behavior_id": behavior_id,
    "behavior_name": behavior_name,
    "step_count": len(steps),
    "resource_count": len(resources),
    "binding_count": len(bindings),
    "steps": step_overview,
    "resources": resource_overview,
    "bindings": binding_overview,
}

summary_path.write_text(json.dumps(summary, indent=2))

blueprint_content = f"""\"\"\"Auto-generated behavior blueprint for {behavior_name}\"\"\"

BEHAVIOR_ID = {json.dumps(behavior_id)}
BEHAVIOR_NAME = {json.dumps(behavior_name)}
STEP_OVERVIEW = {json.dumps(step_overview, indent=4)}
RESOURCE_OVERVIEW = {json.dumps(resource_overview, indent=4)}
BINDING_OVERVIEW = {json.dumps(binding_overview, indent=4)}

def describe_behavior():
    return {{
        "id": BEHAVIOR_ID,
        "name": BEHAVIOR_NAME,
        "steps": STEP_OVERVIEW,
        "resources": RESOURCE_OVERVIEW,
        "bindings": BINDING_OVERVIEW,
    }}
"""

blueprint_path.write_text(blueprint_content)

print("Behavior analysis finished")
print(json.dumps(summary, indent=2))
PY

log "Behavior analysis complete (see $BEHAVIOR_LOG and $SUMMARY_PATH)"
cat "$BEHAVIOR_LOG" >> "$MAIN_LOG"

log "Step 3/5: 采集运行指标"
LLM_METRICS_FILE="$METRICS_DIR/llm_metrics.json"
LLM_METRICS_JSON=""
LLM_CURL_EXIT=0
log "Collecting LLM metrics via curl $LLM_URL/metrics"
set +e
LLM_METRICS_JSON="$(curl -sSL "$LLM_URL/metrics")"
LLM_CURL_EXIT=$?
set -e
if [[ $LLM_CURL_EXIT -ne 0 || -z "$(echo "$LLM_METRICS_JSON" | tr -d '[:space:]')" ]]; then
  log "Warning: 无法获取 LLM metrics（curl exit $LLM_CURL_EXIT），使用占位符"
  LLM_METRICS_JSON="{\"error\":\"LLM metrics unavailable\",\"curl_exit\":$LLM_CURL_EXIT}"
else
  log "LLM metrics 收集完毕，curl exit $LLM_CURL_EXIT"
fi
printf '%s\n' "$LLM_METRICS_JSON" > "$LLM_METRICS_FILE"
log "LLM 运行指标已保存至 $LLM_METRICS_FILE"
cat "$LLM_METRICS_FILE" >> "$MAIN_LOG"

AGENT_HEALTH_FILE="$METRICS_DIR/agent_health_metrics.json"
AGENT_HEALTH_STATUS="000"
AGENT_CURL_EXIT=0
log "Collecting agent health metrics via curl $AGENT_URL/health/metrics"
set +e
AGENT_HEALTH_STATUS="$(curl -sS -w "%{http_code}" -o "$AGENT_HEALTH_FILE" "$AGENT_URL/health/metrics")"
AGENT_CURL_EXIT=$?
set -e
if [[ $AGENT_CURL_EXIT -ne 0 ]]; then
  log "Warning: agent health metrics request failed（curl exit $AGENT_CURL_EXIT）"
  printf '%s\n' '{"error":"agent health metrics unavailable","curl_exit":'$AGENT_CURL_EXIT'}' > "$AGENT_HEALTH_FILE"
  AGENT_HEALTH_STATUS="000"
fi
if [[ "$AGENT_HEALTH_STATUS" -ge 200 && "$AGENT_HEALTH_STATUS" -lt 300 ]]; then
  log "Agent health/metrics 返回 HTTP $AGENT_HEALTH_STATUS (内容在 $AGENT_HEALTH_FILE)"
else
  log "Agent health/metrics 返回 HTTP $AGENT_HEALTH_STATUS（查看 $AGENT_HEALTH_FILE）"
fi
cat "$AGENT_HEALTH_FILE" >> "$MAIN_LOG"

log "Step 4/5: 生成发布说明"
RELEASE_NOTE="$RELEASE_DIR/behavior_validation_note.md"
BEHAVIOR_SPEC_PATH_ENV="$SPEC_PATH"
SUMMARY_PATH_ENV="$SUMMARY_PATH"
LLM_METRICS_PATH_ENV="$LLM_METRICS_FILE"
AGENT_HEALTH_PATH_ENV="$AGENT_HEALTH_FILE"
BLUEPRINT_PATH_ENV="$BLUEPRINT_PATH"
METRICS_DIR_ENV="$METRICS_DIR"
REPO_ROOT_ENV="$REPO_ROOT"
BEHAVIOR_SPEC_PATH_ENV="$SPEC_PATH" SUMMARY_PATH_ENV="$SUMMARY_PATH" LLM_METRICS_PATH_ENV="$LLM_METRICS_FILE" AGENT_HEALTH_PATH_ENV="$AGENT_HEALTH_FILE" BLUEPRINT_PATH_ENV="$BLUEPRINT_PATH" METRICS_DIR_ENV="$METRICS_DIR" REPO_ROOT_ENV="$REPO_ROOT" python3 <<'NOTE' > "$RELEASE_NOTE"
import json
import os
import pathlib

spec_path = pathlib.Path(os.environ["BEHAVIOR_SPEC_PATH_ENV"])
summary_path = pathlib.Path(os.environ["SUMMARY_PATH_ENV"])
llm_metrics_path = pathlib.Path(os.environ["LLM_METRICS_PATH_ENV"])
agent_metrics_path = pathlib.Path(os.environ["AGENT_HEALTH_PATH_ENV"])
blueprint_path = pathlib.Path(os.environ["BLUEPRINT_PATH_ENV"])
metrics_dir = pathlib.Path(os.environ["METRICS_DIR_ENV"])
repo_root = pathlib.Path(os.environ["REPO_ROOT_ENV"])

summary = json.loads(summary_path.read_text())
llm_metrics = llm_metrics_path.read_text().strip()
agent_metrics = agent_metrics_path.read_text().strip()

lines = [
    "# Behavior Validation Release Note",
    "",
    f"- Source spec: {spec_path.name}",
    f"- Behavior ID: {summary['behavior_id']}",
    f"- Behavior name: {summary['behavior_name']}",
    f"- Steps declared: {summary['step_count']}",
    f"- Resources declared: {summary['resource_count']}",
    f"- Bindings evaluated: {summary['binding_count']}",
    "",
    "### Snapshot artifacts",
    f"- Generated plan: {summary_path.relative_to(repo_root)}",
    f"- Blueprint stub: {blueprint_path.relative_to(repo_root)}",
    f"- Metrics bundle: {metrics_dir.relative_to(repo_root)}",
    "",
    "### Metrics (LLM runtime)",
    "```json",
    llm_metrics,
    "```",
    "",
    "### Agent health response",
    "```json" if agent_metrics.startswith("{") else "```text",
    agent_metrics,
    "```",
]
print("\n".join(lines))
NOTE

log "发布说明写入 $RELEASE_NOTE"

log "Step 5/5: 总结输出"
log "验证脚本完成，指标 JSON 将作为最后输出"
printf '%s\n' "$LLM_METRICS_JSON"
