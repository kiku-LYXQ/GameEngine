# Copilot ↔ Agent Interaction Enhancements

## 目标
- 让 Unreal Editor 插件直接利用 Agent 的能力表（capabilities），展示当前 Planner / Code / Asset / Doc Agent 的成功率、平均 latency 与 token 预算。
- 支持 chunk follow-up：选中 RAG chunk（资源路径/Blueprint）后自动将 `chunk_id` 放入 Copilot 请求 headers，使得后端 Agent/LLM 能续写或解释该上下文。

## 实现概要
1. **Agent 改进**
   - `/agents/capabilities`：返回 `AgentCapability` 数组（name、success_rate、avg_latency_ms、avg_tokens），供 UI 展示时参考预计耗时与成本。
   - `/agents/logs/{task_id}`：返回当次任务的 logs（每个 step 及输出），便于 Copilot Panel 或运维 dashboard 用于追踪。
   - Planner 产出的 `AgentTaskStep` 包含 `cost_estimate`（token + latency），便于 Panel 预先展示 `cost_estimate` 字段。
2. **Copilot Panel**
   - Panel 在构造时调用 `RequestCapabilities()`（GET `/agents/capabilities`），并把 response log 写入 logcat；可扩展为 `SListView` 显示具体 stats。
   - 资源卡片可点击 `OnResourceCardClicked`，将 chunk path 与 chunk id 记入 `SelectedChunkId`，后续 `Send` 操作通过 `FCopilotHttpClient` 附带 `X-Copilot-Chunk` header，让后端知道正在 follow-up 哪个 chunk。
   - `FCopilotHttpClient` 现在支持 chunk/request/metadata headers，便于 Agent/LLM 记录 telemetry 并在问题追踪里关联合适 chunk ID。

## 交互顺序
1. Copilot Panel 启动 → 调用 `GET /agents/capabilities` → 显示能力/耗时/token 预算。  
2. 用户选 template/资源 → `SelectedChunkId` 更新。  
3. 发送 prompt → `FCopilotHttpClient` 带上 `X-Copilot-Chunk` + `request_id` + metadata → Agent 处理时可参考历史 chunk；若 Agent 需要解释 chunk 也可回填 chunk metadata。  
4. 后端调用 `/agents/logs/{task_id}` + `/agents/status/{task_id}` → 可在 Panel 侧显示进度。  

## 验证与调试
- 启动 Agent/LLM 服务后，使用 `curl http://127.0.0.1:7000/agents/capabilities` 检查返回。
- Panel 选 chunk 卡片时，查看输出 `Selected chunk: chunk-00X` 以检测 callback 是否生效。
- CLI 调用 `curl http://127.0.0.1:7000/agents/logs/<task_id>` 验证日志格式。
