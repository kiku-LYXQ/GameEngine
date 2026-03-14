# Behavior Pipeline 验证指引

## Context & Objective
- **背景**：行为管线需要一个结构化的 BehaviorSpec 强契约，以驱动 AI Copilot、NPC、文档与行为生成工具。该契约现在通过 `server/agents/behavior_spec.py` 提供，并暴露了 `GET /agents/behavior-spec/{archetype}` HTTP 接口。该接口必须返回包含 `ObjectType`、`RequiredComponents`、`OptionalComponents`、`BehaviorHooks`、`ResourceSlots`、`ValidationTargets` 以及 `Metadata` 七个字段的有效 JSON。
- **目标**：提供 schema+API+prompt 说明，确保在无 LLM 或出错时有明确的 fallback；通过自动化脚本和手动命令验证 schema、API 响应与验证证据的完整性。

## BehaviorSpec Schema & API Validation
1. `pytest server/agents/tests/test_behavior_spec.py`（确保每个 archetype 都能生成含完整字段的 payload，并验证无效 LLM 输出会回退至默认）。将 pytest 输出复制到 `archives/behavior_validation/verification_logs/behavior_spec_pytest.txt` 以作为验收证据。
2. `curl http://127.0.0.1:7000/agents/behavior-spec/character?use_llm=false`（验证 API 在关闭 LLM 的情况下仍然返回合法 schema，并把响应保存到 `archives/behavior_validation/verification_logs/behavior_spec_api.json`）。
3. `curl http://127.0.0.1:7000/agents/behavior-spec/character?use_llm=true`（用于记录 Copilot UI toggle 触发 / 直接 payload 的 LLM 输出）。Save the response to `archives/behavior_validation/verification_logs/use_llm_toggle.json` and log which endpoint you pointed `BEHAVIOR_SPEC_LLM_ENDPOINT` at so the reviewer understands where the LLM runtime lived.

这三条命令分别证明了 schema/swagger/API 默认路径、UI toggle 或 payload 的 LLM 输出、以及 fallback 与验证器的可用性。具体细节、示例 prompt 以及采样输出参见 `docs/system_design/behavior_generation_framework.md`，该文档还说明了 `behavior_spec_example.json` 作为离线样本的用途。

## Automation & Evidence Directories
`archives/behavior_validation/` 汇总了验证过程的产物（禁止覆盖该目录中的现有存档——由 coder3 管理）。该目录由 `scripts/verify_behavior.sh behavior_spec.json` 维护，脚本会以仓库根目录的 `behavior_spec.json` 作为示例输入并生成下述子目录。

在启用 `use_llm` toggle 或调用 `curl ...?use_llm=true` 之前，请先在另一个 shell 中运行 `BEHAVIOR_SPEC_LLM_ENDPOINT=https://free.v36.cm uvicorn server.llm_runtime.app:app --host 127.0.0.1 --port 7001`（可根据需要替换为其他 endpoint + model via env）。脚本期望 `agent` 与 `llm_runtime` 服务在 `127.0.0.1:7000/7001` 上运行，以便将结果写入 `runtime_metrics/` 和 `verification_logs/`。

1. `generated_code/`：脚本产生的 `behavior_plan.json`（行为概览）与 `behavior_blueprint.py` stub，帮助审查绑定、资源与行为步骤的合法性。
2. `verification_logs/`：包括 `behavior_validation.log`（主运行日志）、`code_validity.log`（`python -m compileall` 输出）、`behavior_analysis.log`（结构校验与 summary），以及 `behavior_spec_pytest.txt`、`behavior_spec_api.json`、`use_llm_toggle.json` 报告。启用 Copilot UI toggle 后，保持 `use_llm_toggle.json` 与 `BEHAVIOR_SPEC_LLM_ENDPOINT` 配置的日志也出现在此目录。
3. `runtime_metrics/`：存放 `llm_metrics.json`（LLM 端点）与 `agent_health_metrics.json`（agent 健康指标）。`scripts/verify_behavior.sh` 会触发 `curl http://127.0.0.1:7000/health/metrics`，将 HTTP payload 与状态码写入 `agent_health_metrics.json`，并把两个 JSON 文件同步输出到标准输出以便流水线读取和 release note 继续引用。
4. `release_notes/`：`behavior_validation_note.md` 概述行为 ID、绑定计数、资源表、以及引用的 plan/blueprint/metric 文件与 CLI 结果，是审阅和发布时的摘要证据。

`archives/behavior_validation/behavior_spec_api.json` 建议由 `curl` 命令生成并与验证日志一起存档，以证明 BehaviorSpec API 在禁用 LLM 的情况下依然活跃。

Before you run the automated scripts, verify the Copilot UI behavior_toggle: flip `Use LLM-generated BehaviorSpec`, run `scripts/validate_copilot.sh`, and capture the stdout/stderr in `archives/behavior_validation/verification_logs/copilot_validate.txt`. Complement that with the targeted `curl http://127.0.0.1:7000/api/copilot/generate -H "Content-Type: application/json" -d '{"prompt":"Test use_llm", "use_llm":true}'` call so reviewers can see the request/responses produced with the toggle.

## 验证脚本与实操提示
- `scripts/verify_behavior.sh behavior_spec.json` 会执行完整的校验（行为结构、资源绑定、代码有效性、运行时指标、发布说明），并按照上述目录结构生成文件，使用 `curl http://127.0.0.1:7000/health/metrics` 采集 agent 健康指标并写入 `runtime_metrics/agent_health_metrics.json`。脚本期望 `agent` 与 `llm_runtime` 服务在 `127.0.0.1:7000/7001` 上运行。
- 脚本的 exit code 应为 0，最后一行输出 JSON 格式的运行指标。采集输出后，务必将生成的 `archives/behavior_validation/*` 子目录一起上传或 zip，以便 reviewers 有完整的证据链。
- 如果脚本期间需要验证 BehaviorSpec API，可将 `curl ...?use_llm=false` 的响应另存为 `archives/behavior_validation/behavior_spec_api.json`，并在 `behavior_validation_note.md` 中注明该文件与 `pytest` 输出的位置。
