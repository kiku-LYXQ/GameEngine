# 系统 1：游戏项目知识库（RAG Knowledge Base）

## 目标
1. 支撑跨越 C++、Blueprint、资源、文档的统一知识查询，解决“新人找不到信息”问题。
2. 为 AI Copilot 与 Agent 提供准确上下文，使自然语言问题快速映射到具体文件/节点/资产。
3. 保持数据可追踪、可更新，满足大型项目日常迭代需求。

## 数据采集与处理流水线
1. **扫描层（Project Scanner）：**
   - 遍历 `Source/`, `Plugins/`, `Content/`, `Docs/`，收集 `.cpp/.h/.usf/.uasset/.umap/.md`。
   - 提取文件路径、类型、关联的模块/插件、git hash 作为 metadata。
2. **解析层（Parser + Chunker）：**
   - C++ 使用 `tree-sitter` + 自定义 AST 提取类/函数/注释/调用链。
   - Blueprint 使用 Unreal JSON/UAsset 解析器提取事件图、输入输出、节点说明。
   - 资产通过 `ue4assetparser` 读取 `PackageGuid`, `AssetClass`, `referenced assets`。
   - 文档直接按段落/标题切片，保留上下文和原始链接。
3. **切片策略（Chunking）：**
   - 函数级：保持 `function_name`, `class`, `line_range`, `code_snippet`。
   - Blueprint：以 `graph`, `event`, `function` 为单位，记录 `node_sequence`。
   - 资产：按照 `asset_path`, `tags`, `usage scenarios` 切片。
4. **Embedding + Vector 写入：**
   - 使用 BGE/SeaBert/SentenceTransformer 生成 embeddings，token limit 2048。
   - 所有 chunk 附带 metadata schema（见下表）。
   - 写入 Milvus / FAISS，支持 CPU + GPU Index（IVF + Flat）。

## Metadata Schema（示例）
| 字段 | 说明 |
| --- | --- |
| `path` | 文件/资产全路径（Content/VFX/explosion.uasset） |
| `entity_type` | `cpp_function`, `blueprint_graph`, `asset`, `doc_section` |
| `module` | 所属模块/插件（GameplayAbilities） |
| `line_range` | 代码块行号范围 |
| `tags` | `movement`, `player`, `ability` |
| `hash` | chunk 内容 hash，便于去重与增量更新 |
| `project_version` | git branch + commit |

## 增量更新与同步策略
- 每次扫面都记录 `git commit` + `file hash`，对比差异，并只处理新增/修改 chunk。
- 新文件/Blueprint 触发 chunk 生成，并通过 `Vector delta write` 实现实时同步。
- 旧 chunk 关闭时，更新 metadata 为 `deleted=true`，保留历史用于审计。
- 提供 `sync_status` table 与 `last_synced_at`，便于 UE 插件在请求时提供最新上下文。

## 检索 API
| 接口 | 描述 |
| --- | --- |
| `POST /rag/query` | 输入：`question`, `context_tags`, `max_results`, `project_version`；输出：`chunks[ {path, snippet, score} ]`。
| `POST /rag/context` | 依托 `entity_id` 获取完整 chunk 信息与引用关系。
| `POST /rag/refresh` | 触发 re-embedding，适用于大版本同步。

### 高级功能
- 支持上下文 window + user intent prompt（例如“寻找玩家移动逻辑”会优先返回 `PlayerCharacter.cpp` 的 `MoveForward`）。
- 预置 prompt templates：`explain_blueprint_node`, `find_asset_usage`, `list_related_classes`。
- Cache：对热门问题使用 LRU cache，提升 UE 插件交互流畅度。

## 场景示例：查玩家移动逻辑
1. UE 插件发送 `question=《玩家移动逻辑在哪里》`。
2. RAG API 通过 `tags=[player,movement]` 发现 `Source/Player/PlayerCharacter.cpp` 的 `MoveForward()` chunk。
3. 返回 chunk + snippet，同时附带 `references` 指向其他相关类与 Blueprint。
4. Agent/Plugin 展示路径 + 简要说明，并将 chunk embed 作为后续 prompt context。

## 观测与验证
- **测试**：构建 snapshot dataset，断言 `query` 返回的 `path` 与 `line_range` 与实际代码一致。
- **监控**：Prometheus export chunk ingest rate、query latency、vector DB size。
- **数据质量**：人工校样 pipeline 输出（chunk 预览），确保蓝图/资产 chunk 语义清晰。

## 下一步设计输出
- pipeline 控制台（CLI/GUI）用于查看 scan 状态与 chunk preview。
- Embedding model registry（可切换 BGE/LLama3/Custom）与 fallback 机制。
- RAG 权限控制，区分只读/写入用户。
