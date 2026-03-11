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
- 如果没有 `.uproject` 且也没有 `Config/ Content/ Source/` 这样的 Unreal 工程骨架，说明当前仓库仅包含 AI 插件（ue_plugin/AI_Copilot）、Python 后端（server/）、脚本和 docs，并不是完整的 Unreal 工程。  
- 解决办法：在 Unreal 编辑器中先新建一个 C++ 项目（项目名可为 GameEngine），生成 `.uproject`, `Config/`, `Content/`, `Source/` 目录，再把本仓库的 `ue_plugin/AI_Copilot`、`server/`, `scripts/`, `docs/` 等内容复制/合并进去，避免覆盖 Unreal 自动生成的核心文件。  
- 作为替代，也可以先从一个已有 UE 工程中复制 `.uproject`, `Config/`, `Content/`, `Source/`, `Plugins/` 到这个仓库，然后再把 `AI_Copilot` 插件放到 `Plugins/AI_Copilot` 目录，使当前 repo 成为完整的工程。  

## 1.6. 插件位置与结构
我们的 Unreal 插件源码在：
```
ue_plugin/AI_Copilot/AI_Copilot.uplugin
ue_plugin/AI_Copilot/Source/AI_Copilot/
```
要在 Unreal 工程中使用，请将 `AI_Copilot` 目录复制到工程的 `Plugins/AI_Copilot` 目录。然后运行 `GenerateProjectFiles` + `Build`，生成 `Binaries/`，再启动 Unreal 编辑器并在 Plugins 面板中启用 AI Copilot 插件。  

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
- `scripts/verify_all.sh`: 依赖 `uvicorn` 服务在 7000/7001 端口，顺序运行 `validate_copilot.sh` + `validate_npc.sh` 并输出 `/llm_runtime/metrics`，适合作为 CI/合规验证。
- 只需运行：
  ```bash
  bash scripts/verify_all.sh
  ```
  输出 JSON 就表示链路可用。

## 4. Unreal Editor 使用
1. 确保 Unreal 项目文件 `GameEngine.uproject` 在 `/home/lxy/.openclaw/workspace-architect/GameEngine` 内。用编辑器打开该 `.uproject`，并在 `Plugins` 面板启用 `AI Copilot` 插件，完成后重启编辑器。
2. 若是首次打开/源码更新后没看到按钮，系统会提示 “需要编译”；点击“Yes” 或手动运行 `GenerateProjectFiles` + `Build.bat` 进行 C++ 编译，生成对应 `Binaries/`，确保编辑器能加载插件模块。
3. 重启后工具栏将出现 `AI Copilot` 按钮，也可以通过菜单 `Window > AI Copilot` 打开 Panel。
4. Panel 会自动调用 `/agents/capabilities`、`/agents/npc/task`、`/llm_runtime/metrics`，并在 “Copilot Output Preview” 中显示生成的 summary + 文件。
5. 输入 prompt，点击 “Send to Copilot” 会触发 `/api/copilot/generate`；结果会写入“Copilot Output Preview”和“Execution Logs”，并自动记录 metrics。
6. 只要在同一目录下启动 `uvicorn server.agents.app:app --port 7000` 与 `uvicorn server.llm_runtime.app:app --port 7001`，Panel 就能成功获取数据并执行 prompt。

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