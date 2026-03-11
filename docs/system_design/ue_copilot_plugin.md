# 系统 2：Unreal Editor AI Copilot 插件设计

## 目标
- 让 AI 直接嵌入 Unreal 编辑器，支持生成代码/蓝图、解释资产、搜索资源、快速定位文档。
- 提供可定制的 Slate UI，保持响应式体验并记录上下文操作。
- 与 AI Server（RAG + Agent）通过 HTTP/HTTPS API 低延迟通信。

## 插件架构
```
AI_Copilot Module (Editor)
┌────────────────────┐
│ Slate UI / Toolbar │
└──────────┬─────────┘
           │ HTTPClient
┌──────────▼─────────┐
│ HTTP API Layer     │
│ - request/response │
│ - retry, timeout    │
└──────────┬─────────┘
           │
┌──────────▼─────────┐
│ Local Context Cache│
│ (recent chunks /   │
│  user prompts)      │
└──────────┬─────────┘
           │
┌──────────▼─────────┐
│ Unreal Project APIs│
│ - File creation     │
│ - Slate Notification│
└────────────────────┘
```

### 核心 UI 入口
1. **工具栏按钮**：`AI Assistant`，点击展开 Dockable Panel。
2. **Panel 结构**：
   - Prompt 输入框（支持多行 + 模版切换）
   - 模块 Tabs（Code Generation / Blueprint Explain / Assets / Docs）
   - 结果列表 + Preview 窗口
   - Actions（Copy, Open in Editor, Create Asset）
3. **右键菜单扩展**：
   - 在 Content Browser 上右键 -> `AI Explain Asset`；
   - 在 Blueprint Editor 右键 -> `Ask AI`（sender context automatically appended）。

### HTTP API 设计
| 方向 | Endpoint | Request | Response |
| --- | --- | --- | --- |
| 代码生成 | `POST /api/copilot/generate` | `{ prompt, focus_files, schema: "code" }` | `{ files: [{ path, content }], summary, diagnostics }` |
| 蓝图解释 | `POST /api/copilot/explain-blueprint` | `{ blueprint_id, nodes, question }` | `{ explanation, steps, related_assets }` |
| 资源搜索 | `POST /api/copilot/asset-search` | `{ keywords, filters }` | `{ matches: [{ path, tags, usage_score }] }` |
| 文档导览 | `POST /api/copilot/doc-search` | `{ query, project_context }` | `{ docs: [{ path, excerpt }] }` |

#### 通用约定
- 加入 `project_version`, `module_filter`, `prompt_template` 参数。
- 请求通过 Unreal HTTP Client 发出，支持 5s timeout + 3 retries；失败则提示用户通过 Tray Notification。
- Response 中携带 `page_id` / `chunk_id` 用于后续 `follow-up question`。

### 插件本地行为
- **本地缓存**：保留最近 20 个交互的 prompt/response，允许快速 reopen/undo。
- **上下文注入**：根据当前选中 Actor/Blueprint，附带 metadata（路径、类名、节点 ID）。
- **文件自动生成**：接收到 `files` 后，自动在 `Source/<Module>/AIGenerated` 创建 `.cpp`/`.h`，并调用 `FAssetToolsModule::RegisterAssetTypeActions` 更新项目。
- **蓝图/资产定位**：Response 中返回 `asset_path`，插件调用 `FContentBrowserModule::SyncBrowserToAssets` 聚焦；若返回 `code path`，则打开 `FSourceCodeNavigation`。
- **问题追踪**：在 notifications/log 输出 `request_id`，帮助开发者转换为 bug ticket。

### 错误处理与用户提醒
- 失败重试：HTTP 失败后显示 `Request failed` tooltip，并记录 telemetry（status code, latency）。
- 不达标内容（如无法生成可编译代码）时展示 `AiQuality Warning` 并附带 `Quality Score`。
- 模型/服务不可用时弹窗提示，并建议使用 `Fallback GPT`（e.g., `OpenAI GPT-4` via secure key）。

### 数据上报与安全
- 交互日志写入 `GameDevAI/CopilotLogs`，包含 `prompt_hash`, `response_summary`, `ai_confidence`，方便回溯。
- 插件设置 UI 允许开启/关闭 `prompt telemetry`，并提供 `opt-out` 选项。
- 通过 `IAM` 角色限制 API key 使用，确保只有授权用户可以调用内部模型。

### 场景流程示例（生成冲刺技能）
1. 用户在 Panel 输入“生成玩家冲刺技能”，选择 `Code Generation` 模版。
2. 插件收集 `Current Module`, `Selected Character BP` 作为 context，POST `/api/copilot/generate`。
3. AI Server 通过 Agent Orchestrator 查询 RAG，调用 LLM 生成 `UAbility_Sprint` 类。
4. 插件收到 response，创建文件、注册组件、刷新 visual log。
5. Panel 显示 Summary + Compile button；同时上传 artifact to Agent for future reference。

## 下一步产出
- 补充 `docs/system_design/ui_wireframes.md`（Slate Panel 线框）
- 形成 API contract 文档（OpenAPI/JSON schema）
- 列出 `Blueprint Explain` 的解析流程与 node 转换表
