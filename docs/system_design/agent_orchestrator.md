# 系统 3：AI 开发 Agent 编排

## 目标
- 将复杂开发任务拆解为多个角色明确的 Agent（Planner / Code / Asset / Doc），并通过共享上下文与状态实现可审计的自动化。
- 提供一致的输入/输出 contract 与追踪机制，以便 UE 插件、CLI 或 API 调用统一触发。
- 支撑多轮对话、任务中断与补充指令。

## Agent 体系结构
```
User Prompt
    │
FastAPI / WebSocket Endpoint
    │
Agent Orchestrator
    ├── Planner Agent
    ├── Code Agent
    ├── Asset Agent
    └── Doc Agent
Shared Context Store (Redis/Postgres)
```

### Agent Contract 规范
| Agent | 输入 | 输出 | 依赖资源 |
| --- | --- | --- | --- |
| Planner | `prompt`, `project_context` | `task_plan`（steps + priorities + dependencies） | RAG context tokens, capability list |
| Code | `task_step`, `code_context`, `project_files` | `patches`, `compile_commands`, `diagnostics` | RAG chunks, LLM Runtime, Compiler info |
| Asset | `task_step`, `asset_constraints` | `asset_matches`, `usage_scores` | Vector Asset Index, Content browser metadata |
| Doc | `task_step`, `summary_context` | `design_doc`, `task_notes` | RAG docs, previous agent logs |

## 典型流程：实现“敌人 AI”
1. Planner Agent：
   - 检索 RAG 中 `Enemy`、`AI`、`Behavior Tree` 相关 chunk；
   - 拆分需求为：1) 行为树设计 2) C++ 控制 3) 资源配置 4) 文档说明。
2. Code Agent：
   - 请求 LLM Runtime 生成 `AEnemyAIController` + ability `UAbility_Chase`；
   - 输出 patch（`.h`/`.cpp`）与 `Build` 命令建议。
3. Asset Agent：
   - 查询 Vector Asset Index，返回爆炸粒子 + 音效路径；
   - 提供 `.uasset` 路径与 `usage_score` 供 UE 插件选用。
4. Doc Agent：
   - 构建任务说明，包括输入/输出与调用示意图；
   - 生成 Markdown（可上传至 `docs/AI-Tasks/enemy-ai.md`）。
5. Orchestrator 合并所有输出，形成最终 `AgentTaskResult` 返回给 Caller。

## 状态管理与审计
- 所有 Agent 与请求 ID 绑定，记录 `start_ts`, `end_ts`, `response_size`, `tool_calls`。
- 使用 PostgreSQL/Redis 保存 `Context Slot`，以支持多轮（Multi-turn）交互。
- 每个 Agent 输出 `quality_score` 与 fallback suggestion（如 LLM output low confidence 会 fallback to human review）。

## 安全与可控
- 所有 Agent 通过 `Capability Matrix` 限制调用接口（`Code Agent` 不能直接修改仓库，需要人类确认）。
- 审计日志（OpenTelemetry）记录 `prompt_hash`, `agent`, `actions`，并上报到 Grafana。
- 支持 `manual approval` step，在 `AgentTask` 状态中插入 `review_needed` tag。

## 接口与延展
- `/agents/task`（POST）：接受 prompt + requirements，返回 `task_id` 与 `status`。
- `/agents/status/{task_id}`：轮询 agent 状态与 logs。
- `/agents/feedback/{task_id}`：为误判/失败生成追踪 ticket。
- 支持 `webhooks`，允许 CI/CD pipeline 在 Agent 完成后自动创建 PR 或 issue。

## 监控/验证
- 为每个 Agent 设置 `latency`, `error_rate`, `success_ratio` 指标。
- 定期通过 `Smoke Test` Prompt（“生成一个简单Actor”）检验 pipeline 健康。
- 为 Agent 生成 `cost_estimate`，便于算力规划（如 LLM tokens, embedding cost）。

## 下一步输出
- 编写 `docs/system_design/agent_flow_sequence.md`，包含 UML sequence diagram。
- 梳理 Agent 之间的 `context schema`（key/value map）和 `fallback/error handling policy`。
