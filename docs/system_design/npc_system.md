# AI NPC System for GameDev AI Platform

## 1. 目标
- 让 NPC 具备对话、任务与行为树决策能力，并通过现有 Agent/RAG/LLM 平台生成 NPC 脚本。 
- 提供 NPC Agent，聚合行为上下文、蓝图、资产与 LLM 输出，能够在 Unreal 侧被 Copilot 插件触发。
- 给团队提供可观测、可验证的 NPC 生成流程（capabilities、logs、metrics）。

## 2. 架构概览
```
NPC Behavior Tree Panel
        │
      HTTP request
        │
  NPC Agent (Planner + Dialogue + Task)
        │
    ┌───┴───┐
    │  RAG  │
    └───────┘
        │
   Vector DB + Metadata
        │
   ┌──────────────┐
   │  LLM Runtime  │
   │ (npc-dialogue)│
   └──────────────┘
        │
     LoRA trainer
```

## 3. NPC Agent 工作流
1. **请求触发**：UE 插件（Behavior Tree Panel/Dialogue Console）发起 `POST /agents/npc/task`，输入 `npc_id`, `behavior`, `intent`, `chunk_id`。
2. **Planner agent**：解析意图并拆分 `dialog`, `task`, `behavior` 三个子任务，交给对应 Agent：
   - Dialogue Agent：调用 RAG 获取 NPC 背景（personality, lore），通过 LLM Runtime 生成对话行。
   - Task Agent：查纹理/资产，确定任务目标，实现 `npc quest`。
   - Behavior Agent：输出行为树节点配置（例如 `MoveTo`, `PlayAnimation`），可映射至 UE Behavior Tree assets。
3. **Context store**：保存 `task_id`, 所有关联 chunk/topic, 成功率、耗时；同样为 Copilot Panel 的 capability list 提供数据。
4. **输出**：返回 `dialogue_lines`, `task_script`, `behavior_plan`，同时写入 `/agents/logs/{task_id}` 供 UI 展示。

## 4. NPC模型与 LLMOps
- 注册新模型 `npc-dialogue-7b`/`npc-task-7b`，LoRA 训练使用 `npc_dataset`（任务 + 对话 + Blueprint 语料）。
- 提供 `/models/{name}/evaluate`，输入 NPC prompt + expected behavior 输出 `metrics`（coherence, safety, compile success）。
- 支持 fallback chain：先用 `npc-dialogue` 生成对话，再切换到 `game-qwen-7b` 生成 C++/Blueprint 代码，最后组合成 NPC artifact。

## 5. Unreal 插件扩展
- Behavior Tree Panel：展示行为树节点，可选 `chunk`/`template` → 发送 `NPC Agent` 请求；支持连带 `chunk_id`/`request_id` headers
- Dialogue Console：展示生成的对话行 + 任务描述，提供 “apply to Behavior Tree” 操作。
- UI 同样可请求 `/agents/capabilities` 了解 NPC Agent 成功率指标。

## 6. 验证与监控
- 新增 `scripts/validate_npc.sh`：调用 `/agents/npc/task`, `/agents/npc/logs/{task_id}`, `/models/npc-dialogue-7b/evaluate`, `/llm_runtime/metrics`。
- 监控指标：NPC dialog success ratio（Task logs）、LLM token usage、behavior plan latency。
- 测试场景：`GenerateNPCDialogue` → `PlanNPCQuest` → `ApplyBehavior`，确保 Behavior Tree Panel 能滚动展示 logs + follow-up prompts。
- 可运行 `scripts/validate_npc.sh`（依赖 `jq`）自动完成 NPC task + logs + model evaluate + metrics 验证。
