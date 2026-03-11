# 系统 4：LLMOps 与本地模型平台

## 目标
- 提供面向游戏开发场景的本地推理服务，兼容 OpenAI API，同时支持 LoRA 微调和模型监控。
- 为 AI Server、UE 插件、Agent 提供统一的模型调用入口，并追踪成本与性能。

## 模型服务层次
1. **Inference Layer**
   - 使用 vLLM/Deepseek/Code LLM（如 Mixtral, Qwen-7B）做推理；支持 FP16/BF16。
   - 提供 Streaming（SSE + WebSocket）与 Blocking API。
2. **API 层**
   - FastAPI 实现 `/v1/completions`, `/v1/chat/completions` 接口，兼容 OpenAI 请求参数（prompt, messages, temperature, max_tokens）。
   - 支撑 `user_id`, `project_id` metadata，用于多租户控制与费用核算。
3. **LoRA 微调层**
   - LoRA Trainer 基于 PEFT/BitsAndBytes，接收 `dataset/game_project` 生成的 UE 代码 chunk 训练 LoRA checkpoints。
   - 输出 `.safetensors` + config，注册到 Model Registry。
4. **Model Registry + Deployment**
   - 维护表：`models (name, version, base_model, lora_path, status)`。
   - 自动根据 `status` 选定 CPU 或 GPU execution layer。

## API 兼容行为
| 原始参数 | 解释 | 支持情况 |
| --- | --- | --- |
| `messages` | 仅 Chat 模式，系统/用户/assistant role | ✅ | 
| `functions` | 提供 function calling stub（可扩展） | ⚠️ 依赖 LLM Capability |
| `stream` | 是否流式返回 | ✅ SSE/WebSocket 支持 |
| `temperature` | 控制 creativeness | ✅ |
| `model` | LoRA 版本支持 `game-sprint-001` | ✅ |

- 对外暴露 `OpenAI-compatible API key`，并把 key 与 `project scope` 绑定。
- 内部会将每个请求记录 `token_usage`、`prompt_context`, `response_length`。

## LoRA 微调流水线
1. 数据准备：使用 `dataset/game_project` 中 chunk + metadata，筛选 `UE code` + `Blueprint doc` 句子。
2. LoRA Trainer：调用 `peft.train`, `bitsandbytes` 进行低秩适配，支持 8bit/4bit。
3. 验证：在 `llm_runtime/tests` 运行 prompt benchmark（“生成 ability class”）并对比质量指标（perplexity, Rouge, compile success）。
4. 发布：将 checkpoint push 到 Model Registry，并在 API 中新增 `model` entry。

## 监控与成本控制
- Prometheus metrics：`llm_requests_total`, `llm_latency_seconds`, `gpu_memory_usage`, `lora_training_jobs`。
- Grafana dashboard shows token usage, latencies by project, quality warning rate.
- 定期清理 stale models，保持 storage/compute budget。

## 安全与审计
- 所有模型加载必须通过签名验证（确保 `.safetensors` 未被篡改）。
- LoRA 训练 pipeline 支持数据审核，敏感文件将被 exclude。
- 请求中 `user_id`/`project_id` 作为 metadata 传递，便于 traceability。

## 未来拓展
- 实现 `model chaining`，在 Agent pipeline 中先用 fast chat model，然后调用 Code LLM 生成代码。
- 增加 `fallback to OpenAI` policy，当本地模型负载过高自动切换。
- 提供 `model drift alert`（token usage vs success ratio）来触发 re-train。
