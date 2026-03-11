# AI Copilot Unreal Plugin

## 目录结构
- `AI_Copilot.uplugin`：插件描述
- `Source/AI_Copilot`：C++ 模块代码
- `Source/AI_Copilot/Public`：对外接口
- `Source/AI_Copilot/Private`：Slate 面板与 HTTP helper

## 模块说明
- `AI_Copilot.Build.cs`：声明依赖模块（Slate、Http 等）
- `AI_Copilot.cpp/h`：模块生命周期，当前仅输出 log（可扩展为注册 toolbar、Slate UI）
- `CopilotPanel`：Slate 面板 placeholder，可在未来加入 Slate 构建逻辑
- `CopilotHttpClient`：负责向 `api/copilot/*` 发起请求

## HTTP API 模板
### 1. 代码生成
```json
POST /api/copilot/generate
{
  "prompt": "Create a player sprint ability in C++",
  "context": {
    "module": "Gameplay",
    "files": ["Source/Player/PlayerCharacter.cpp"]
  },
  "schema": "code",
  "max_tokens": 512
}
```
> Response:
```json
{
  "files": [
    {"path": "Source/Gameplay/Abilities/UAbility_Sprint.cpp", "content": "..."}
  ],
  "summary": "Generated sprint ability + registration",
  "diagnostics": []
}
```
### 2. 蓝图解释
```json
POST /api/copilot/explain-blueprint
{
  "blueprint_id": "BP_PlayerCharacter",
  "nodes": ["MoveForward", "Jump"],
  "question": "这个蓝图在做什么？"
}
```
### 3. 资源搜索
```json
POST /api/copilot/asset-search
{
  "keywords": ["explosion", "impact"],
  "filters": {"type": "particle"}
}
```
> Response sample:
```json
{
  "matches": [
    {"path": "Content/VFX/Explosions/explosion_fx.uasset", "usage_score": 0.96}
  ]
}
```

## 错误处理与埋点
- 每次调用用 `FCopilotHttpClient::PostRequest` 包装，并传入 `request_id` + `context`，失败时在 editor log 中输出 friendly message，必要时弹出 notification。
- HTTP 超时或状态码非 200 时，重复请求 2 次（exponential backoff），如果仍失败则提示“AI 服务暂不可用”。
- 插件在 `GameDevAI/CopilotLogs` 写入 `prompt_hash`, `response_hash`, `latency_ms`，并允许开发者通过设置关闭 telemetry。

## 模板与资源卡片
- Copilot Panel 提供预置模板（Sprint Ability、Dialogue Event 等），点选模板可以自动注入参数与 context，并展示推荐资源路径（Blueprint、VFX、音效）。
- Panel 允许将 chunk_id（由 Agent 的 RAG responses 返回）附加在请求头中，后续 prompt 会自动 carry chunk 的 metadata，便于 follow-up question 与 chunk-based explanations。
- `FCopilotHttpClient` 通过 headers 传递 `X-Copilot-Chunk`, `X-Copilot-Request-Id`, `X-Copilot-Metadata`，可在后端 Agent/LLM logs 中追踪用户行为。
- 点击资源卡片会直接将路径填入 Copilot prompt input，或触发 `Content Browser` 的 `SyncBrowserToAssets`（未来可实现），让开发者快速 jump to asset。

## 交互流程建议
1. 从 Toolbar 打开 Copilot Panel，输入 prompt 并选择模块。
2. Panel 初始化时调用 `/agents/capabilities`，将 success rate / avg latency 展示在 UI，帮助开发者选择合适 Agent。
3. 模块在本地构造 `context`（selected actor/blueprint paths）并调用 `/api/copilot/generate`，请求头包含 `X-Copilot-Chunk`（若选了资源卡片）与 `X-Copilot-Request-Id`，后端可据此执行 follow-up。
4. 服务返回时，自动创建/打开文件或 focus 资源，并把 agent result push 至 `CopilotLogs`。
5. 若响应中包含 `chunk_id`，下一个 prompt 可以作为 follow-up 直接引用。参考 `docs/system_design/copilot_agent_link.md` 了解具体 headers/methods。

## Debug 与验证
- 使用 `FCopilotHttpClient::PostRequest` 发起 `POST /api/copilot/generate` 请求，观察是否可以成功接收到 `files`，并调用 `FAssetToolsModule::RegisterAssetTypeActions` 创建资源。
- 可通过 `curl http://127.0.0.1:7000/api/copilot/generate` 模拟请求，确保本地 LLM Runtime + Agent 正常响应。
- 启动服务后 `curl http://127.0.0.1:7000/agents/capabilities` 应返回 capabilities JSON，便于 UI 侧展示。
- 可运行 `scripts/validate_copilot.sh`（需 `jq`）自动执行 agent task、logs、copilot API 与 LLM metrics 验证，确保整个链路一致。
