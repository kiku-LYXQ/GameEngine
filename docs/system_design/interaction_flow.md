# Interaction Flow & Monitoring

## 1. 端到端调用链
(详见 `docs/system_design/copilot_agent_link.md` 了解 Copilot ↔ Agent 的新增交互)
```text
[Unreal Editor (AI Copilot UI)] -> POST /api/copilot/generate
           ↓
     [Agent Orchestrator]
           ↓                ┌────────────────────┐
     [Planner Agent] ◀─────┤ RAG Knowledge Base │
           │                └────────────────────┘
[Code/Asset/Doc Agents] ──▶│ Vector DB + Embeddings │
           ↓                     └──────▲────────────┘
         requests                embeddings + metadata
           ↓
     [LLM Runtime / LLMOps]
           ↕
   (/v1/completions, /v1/chat/completions)
           ↓
   (Local model or external proxy)
```

- **UE 插件** 提供 Slate UI + HTTP helper，发送 prompt + context 至 `/api/copilot/generate`。
- **Agent Orchestrator** 依次触发 Planner → Code/Asset/Doc，Planner 调用 RAG 检索 chunks（Chunk metadata + vector）并返回 `task_plan`。
- **Agent** 负责生成代码片段、查找资产（Vector Asset Index）与撰写设计文档，同时调用 LLM Runtime 的 `/v1/completions` 生成实际文本。
- **LLM Runtime** 若配置了 `EXTERNAL_LLM_ENDPOINT`，会先代理请求到第三方（如 `https://free.v36.cm`）；否则使用本地 stub。返回结果反馈给 Agent，再统一返回 UE 插件。

## 2. 数据与控制流
| 层 | 输入 | 关键输出 | 存储/通信 | 监控指标 |
| --- | --- | --- | --- | --- |
| Copilot UI | prompt + context (selected Blueprint/Actor) | `task_id` + `request_id` | HTTP | UI latency, retry count |
| Agent Orchestrator | prompt, context | plan + step logs | Redis/Postgres context store | agent success ratio, `task_duration` |
| RAG KB | request tags/project_version | chunk list (`path`, `snippet`, `score`) | Milvus/FAISS, metadata table | query latency, chunk hit rate |
| LLM Runtime | messages/completion request | generated text or forward response | FastAPI + `metrics_store` counters | `completions.requests`, `chat.responses`, `lora.jobs_started`, external proxy errors |
| Vector DB | chunk embeddings | filtered context for Agent | Milvus/FAISS | index size, ingestion rate |

## 3. Monitoring & Observability
- **RAG Query Latency**：记录 `/rag/query` 响应时间；超过 `500ms` 触发 alert。
- **Agent Success Ratio**：定期运行 smoke test（`server/agents/smoke_test.py`），计算 `done` vs `failed`；报警 threshold < 90%。
- **Agent Capability Dashboard**：`/agents/capabilities` 提供 Planner/Code/Asset/Doc 的 success_rate, avg_latency_ms, avg_tokens；由 UE 插件或运营 dashboard 拉取，帮助前端展示预计耗时与 token 预算即可。`/agents/logs/{task_id}` 提供每个 step 的 log/result string，便于后续调试。
- **LLM Token Usage**：`metrics_store` 中 `completions.responses` / `chat.responses` 可视化，外部 proxy 增加 `external.failed` counter。
- **LoRA Training Jobs**：`/models/{name}/lora` 触发后 `metrics_store` 记录 `lora.jobs_started`，可追踪 `LoRA job duration`。
- **External Proxy Health**：若 `_forward_to_external` 返回 `None`，会在 log 中产生日志 `External LLM request failed`，并记录 `external.failed` counter（后续可扩展）。

## 4. Smoke Test / Verification Commands
- `scripts/validate_copilot.sh` 可作为一键 smoke test：依次调用 `/agents/capabilities`, `/agents/task`, `/agents/status`, `/agents/logs`, `/api/copilot/generate` 以及 `/llm_runtime/metrics`，并打印 JSON 结果，方便确认 Copilot → Agent → LLM 的链路。
- `scripts/validate_npc.sh` 验证 NPC Agent 与 NPC LLM 模型：执行 `/agents/npc/task`, `/agents/logs/{task_id}`, `/models/npc-dialogue-7b/evaluate`, `/llm_runtime/metrics`，并记录输出。
- `docs/getting_started.md` 汇总了如何启动服务、运行验证脚本、打开 Unreal Copilot Panel 与查看 metrics，适合新手直接照着操作。
```bash
# 1. 启动 Agent + LLM Runtime
uvicorn server.agents.app:app --port 7000
uvicorn server.llm_runtime.app:app --port 7001

# 2. 运行 smoke test，展示全链路调用
python server/agents/smoke_test.py

# 3. 观察 metrics
curl http://127.0.0.1:7001/metrics

# 4. 模拟 Copilot HTTP 请求
curl http://127.0.0.1:7000/api/copilot/generate -X POST \
  -H "Content-Type: application/json" \
  -d '{"prompt": "Create a sprint ability", "context": {"module": "Gameplay"}}'
```

产出结果:
- `smoke_test.py` 会打印 `task_id`、轮询 `status`、展示 `agent` logs 与 `metrics`。
- `curl /metrics` 显示 `completions.requests`, `chat.responses`, `lora.jobs_started`。
- 若 `/api/copilot/generate` 还未实现，调用失败可用于后续 Agent -> API 路径留痕。
