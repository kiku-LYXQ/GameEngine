# RAG Knowledge Base

## 目标
- 为 Unreal 项目提供快速、高精度的自然语言知识检索。
- 覆盖代码、Blueprint、资产与设计文档。

## 组件
1. **Project Scanner**：递归 UE 项目路径，收集 `Source/`, `Content/`, `Docs/`。
2. **Parser/Chunker**：基于 `tree-sitter` 与 Blueprint 解析器，将代码与节点按函数/块切分。
3. **Embedding Pipeline**：BCD embedding、BGE、text2vec、OpenAI embedding 兼容，输出 vector + metadata。
4. **Vector Store**：Milvus / FAISS，按 `file_path`, `entity_type`, `context` 做索引。
5. **Retrieval API**：Rest API 服务，支持上下文过滤与命令式 prompt（例如“找角色移动”）。

## Python 骨架说明
- `server/rag/app.py`：FastAPI 应用，提供 `/rag/query`、`/rag/context`、`/rag/refresh` 等接口。可直接通过 `uvicorn server.rag.app:app --reload` 启动。
- `server/rag/vector_store.py`：内存级 stub，支持筛选 tag、project_version，并暴露近似查询接口。
- `server/rag/embeddings.py`：占位 embedding，未来替换为真正模型（BGE/LLM）。
- `server/rag/models.py`：定义 Pydantic schema，供前端和 Agent 调用。

## 预留项
- 数据更新策略（文件增量扫描 + Bloom Filter）
- Metadata Schema（`{path, type, tags, line_range, hash}`）
- 验证：通过集成测试调用 API 返回正确文件路径。
