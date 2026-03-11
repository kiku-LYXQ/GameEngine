# Getting Started with AI GameDev Platform

本指南帮助“新手”一步步启动 AI Copilot + NPC Flow，涵盖目录说明、服务启动、验证脚本与 Unreal 交互。

## 1. 目录结构简介
- `server/agents`: FastAPI 提供 `/agents/task`, `/api/copilot/*`, `/agents/npc/task` 等能力。  
- `server/llm_runtime`: LLMOps 本地推理 + LoRA eval，暴露 `/v1/completions`, `/models/{name}/evaluate`, `/metrics`。  
- `ue_plugin/AI_Copilot`: Unreal 编辑器插件，包含 Copilot Panel Slate UI + HTTP helper。  
- `scripts/validate_copilot.sh`, `scripts/validate_npc.sh`: 一键 smoke test。  
- `docs/system_design`: 详细架构、数据流与交互说明。  

## 1.5. 判断 Unreal 工程结构
在本地目录（建议仍使用 `/home/lxy/.openclaw/workspace-architect/GameEngine`）运行：
```
find . -maxdepth 2 -name "*.uproject"
find . -maxdepth 2 -type d | sort
```
- 如果没有 `.uproject` 且也没有 `Config/ Content/ Source/` 这样的 Unreal 工程骨架，就说明该仓库当前只有插件、后端、脚本和 docs，并不包含完整的 Unreal 工程。  
- 解决办法：在 Unreal 编辑器中先新建一个 C++ 项目（项目名可为 GameEngine），生成 `.uproject`、`Config/`、`Content/`、`Source/`。  
- 然后把本仓库的 `ue_plugin/`, `server/`, `scripts/`, `docs/` 内容合并到这个新建工程的目录里，避免覆盖 Unreal 自动创建的核心文件；再把 `.uproject` 所在目录作为新仓库 root 进行 Git 操作。  
- 如果你希望直接基于本仓库继续操作，也可以在其他位置创建 `.uproject` 工程后，将文件如 `Config/`, `Content/`, `Source/`, `Plugins/` 等复制/移动到此仓库，确保 Unreal 识别该目录为工程。  

之后再继续下面章节（服务启动、脚本验证、Unreal 编辑器使用）。

## 2. 启动本地服务
1. 在 `GameEngine` 目录打开终端，运行：
   ```bash
   uvicorn server.agents.app:app --port 7000 --reload
   ```
2. 新终端继续在项目目录运行：
   ```bash
   uvicorn server.llm_runtime.app:app --port 7001 --reload
   ```
3. 旁边可打开 `tail -f /tmp/agent.log` / `tail -f /tmp/llm.log` 观察请求。

## 3. 一键验证脚本
- `scripts/validate_copilot.sh`: 验证 Agent → Copilot → LLM（capabilities → task → `/api/copilot/generate` → `/metrics`）；需要 `jq`。
- `scripts/validate_npc.sh`: 验证 NPC Agent + LLM eval；记录 `evaluation.requests` 等指标。
- 只需运行：
  ```bash
  bash scripts/validate_copilot.sh
  bash scripts/validate_npc.sh
  ```
  输出 JSON 就表示链路可用。

## 4. Unreal Editor 使用
1. 确保 Unreal 项目文件 `GameEngine.uproject` 在 `/home/lxy/.openclaw/workspace-architect/GameEngine` 内。用编辑器打开该 `.uproject`，并在 `Plugins` 面板启用 `AI Copilot` 插件，完成后重启编辑器。
2. 重启后工具栏将出现 `AI Copilot` 按钮，也可以通过菜单 `Window > AI Copilot` 打开 Panel。
3. Panel 会自动调用 `/agents/capabilities`、`/agents/npc/task`、`/llm_runtime/metrics`，并在 “Copilot Output Preview” 中显示生成的 summary + 文件。
4. 输入 prompt，点击 “Send to Copilot” 会触发 `/api/copilot/generate`；结果会写入“Copilot Output Preview”和“Execution Logs”，并自动记录 metrics。
5. 只要在同一目录下启动 `uvicorn server.agents.app:app --port 7000` 与 `uvicorn server.llm_runtime.app:app --port 7001`，Panel 就能成功获取数据并执行 prompt。

## 5. 常见检查点
| 目标 | 检查命令/位置 |
| --- | --- |
| Agent 是否在线 | `curl http://127.0.0.1:7000/agents/capabilities` |
| Copilot 是否可用 | `curl http://127.0.0.1:7000/api/copilot/generate -d '{...}'` |
| NPC 行为计划 | Copilot Panel 中的 "NPC Behavior Plan" 看 behavior_plan + dialogue lines |
| LLM metrics | `curl http://127.0.0.1:7001/metrics`、Panel 顶部 badges |
| 验证脚本 | `scripts/validate_copilot.sh` + `scripts/validate_npc.sh` 输出 200 JSON |

## 6. 术语说明
- **Chunk**：RAG 里的 chunk 表示代码/资产片段，Copilot 可以带 `chunk_id` 做 follow-up。  
- **NPC Agent**：负责 `dialogue`, `task`, `behavior`，输出 behavior_plan + dialogue_lines。  
- **LoRA Evaluate**：调用 `/models/npc-dialogue-7b/evaluate` 返回 coherence/safety/compile score。  
- **Metrics Badge**：Copilot Panel 顶部的 metrics text 来自 `/llm_runtime/metrics`，显示评价/调用次数。  

## 7. 下一步
- 试着点开资源卡片观察 prompt auto-fill；  
- 用脚本查看 logs/metrics 展示在 Panel；  
- 修改 prompt 后重新跑 `scripts/validate_copilot.sh`，观察 summary/files 是否更新。

如需我再提供图示、视频或 README 截图供分享，请告诉我我可以立即补充。