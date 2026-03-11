# LLM Runtime / LLMOps

## 目标
- 管理本地推理模型（Deepseek、Qwen、Code LLM），并提供与 UE 插件、Agent 统一的调用接口。
- 提供 LoRA 微调入口，使模型更擅长 UE 代码 + Blueprint 表达。

## 组件
1. **Inference Layer**：基于 vLLM，兼容 huggingface transformers 模型和 safetensors。
2. **API Layer**：FastAPI 实现 `/v1/chat/completions`, `/v1/completions`, 兼容 OpenAI 协议。
3. **LoRA Trainer**：接收项目代码数据，通过 PEFT/BitsAndBytes 训练 LoRA 并输出 checkpoint。
4. **Monitoring**：Prometheus Exporter + Grafana Dashboard，实时展示调用量、延迟、GPU 利用率。

## 支持
- Token Streaming（Server Sent Events / WebSocket）
- Context Window Management
- Model Registry（包括 config, tokenizer, checkpoint）

## 里程碑
- 接入至少一个本地模型
- 实现 OpenAI API 兼容层
- 完成 LoRA 推理 + 推理性能指标
