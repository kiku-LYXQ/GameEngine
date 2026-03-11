# Agent Orchestrator

## 目标
- 基于用户意图，协调 Planner、Code、Asset、Doc 等 Agent 实现复杂需求。
- Agent 之间通过共享 state/context 交换信息，并记录执行痕迹。

## Agent 模式
1. **Planner Agent**：解析用户 prompt，调用 RAG KB 查询相关背景，输出任务列表。
2. **Code Agent**：负责实际代码生成（C++/Blueprint）并产出 patch/文件结构。
3. **Asset Agent**：在 vector assets index 中寻找贴图、粒子、音效等引用。
4. **Doc Agent**：生成设计文档、行为描述、调用说明。

## 技术栈
- LangGraph / AutoGen 任务流水线
- FastAPI + Celery（可选）作为调度器
- Redis + PostgreSQL 用于状态与审计日志
- OpenTelemetry for tracing

## FastAPI 骨架
- `server/agents/app.py`：提供 `/agents/task`, `/agents/status/{task_id}`, `/agents/capabilities`, `/agents/logs/{task_id}`, `/agents/feedback/{task_id}` 等接口。
- `server/agents/context.py`：简单 ContextStore，记录状态与审计日志。
- `server/agents/models.py`：定义 Agent contract + Task Result/Status schema，以及 `AgentCapability`/`TaskLogs` 模型。
- `server/agents/agents/*`：Planner/Code/Asset/Doc Agent stub，可直接拓展为真实逻辑。
- `server/agents/smoke_test.py`：闭环脚本，验证 Agent → LLM Runtime → metrics 的调用链，并打印 logs + metrics for diagnostics。

## Agent Capability Dashboard
- `GET /agents/capabilities`：返回 Planner/Code/Asset/Doc/NPC 的 success_rate、avg_latency_ms 与 avg_tokens，可直接用于 UE 插件或运维 dashboard 展示。
- `GET /agents/logs/{task_id}`：查看指定任务的执行日志（step + outputs），便于排查失败或验证 run history。
- Planner 生成的 `task_plan` 已附带 `cost_estimate`（tokens + latency），可用于前端展示预计耗时/Token 粒度。更多 Copilot ↔ Agent 的交互细节见 `docs/system_design/copilot_agent_link.md`。
- `POST /agents/npc/task`：NPC Agent 接收 `npc_id`, `behavior`, `intent`, `chunk_id`，生成 dialogue + task plan + behavior tree nodes；参考 `docs/system_design/npc_system.md` 了解完整流程。

## 下一步
- 定义 Agent contract（inputs/outputs）
- 增加安全审核 hook
