# LLM Runtime / LLMOps

## 背景
`server/llm_runtime` 承载 GameDev AI Platform 的本地推理与模型运维。代码提供一个 OpenAI API 兼容层、模型注册中心、LoRA 微调入口，以及基础指标采集，便于 UE 插件 / Agent 统一调用。

## 目录指南
- `app.py`：FastAPI 应用，暴露 `/v1/completions`, `/v1/chat/completions`, `/models`, `/models/{name}/lora`, `/metrics` 等接口。接口使用 `models.py` 中的 Pydantic schema，返回模拟结果。
- `models.py`：定义 Completion/Chat/Model/LoRA 请求与响应 schema，保持与 OpenAI 兼容结构。
- `registry.py`：内存 ModelRegistry，可注册/查询模型。默认注册 `game-qwen-7b`、`game-deepseek-13b`。
- `lora.py`：LoRA Job 管理器，用于调度({`started_at`, `job_id`} ) stub。Logs + metrics 由 `metrics.py` 触发。
- `metrics.py`：简单的 counter store，可在 `/metrics` 查看 `completions.requests`, `chat.responses`, `lora.jobs_started` 等。
- `tests/test_app.py`：使用 FastAPI TestClient 验证核心端点能正确返回 HTTP 200。

## 运行方式
```bash
pip install -r server/llm_runtime/requirements.txt
uvicorn server.llm_runtime.app:app --host 0.0.0.0 --port 7001
```

## 接口速览
| Endpoint | 描述 |
| --- | --- |
| `POST /v1/completions` | 接收 text prompt，返回 `choices` + usage。|
| `POST /v1/chat/completions` | 接收 messages，返回 chat completion。|
| `GET /models` | 列出已注册模型。|
| `POST /models/{name}/lora` | 触发 LoRA 训练 job，返回 job_id + status。|
| `GET /metrics` | 输出计数指标，便于 Prometheus 抓取。|

## 下一步
- 连接真实 vLLM / HuggingFace 模型，替换 `_build_choice` 逻辑。
- 将 LoRA job 与训练 pipeline（PEFT + dataset）对接。
- 增加 token streaming（SSE/WebSocket）与函数调用能力。
