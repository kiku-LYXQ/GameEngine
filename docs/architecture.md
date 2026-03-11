# AI GameDev Platform Architecture

“AI GameDev Platform” 旨在为大型游戏团队打造可以落地使用的全链路开发工具，围绕 **Unreal 编辑器插件、AI Server（RAG + 多 Agent + LLM 运行时）、本地模型部署与 Vector DB** 构建立体架构，将 AI 工具直接嵌入 Unreal 开发流程中。

## 1. 核心架构

```
Unreal Editor Plugin (AI Copilot)
        │
      HTTP(S)
        │
  AI Server / Agent Orchestrator
 ┌────┴────────┐
 │   RAG KB    │
 │ LLM Runtime │
 └─────────────┘
        │
 ┌──────┴────────┐
 │ Vector DB +   │
 │ Asset Index   │
 └───────────────┘
```

### 核心组件
- **Unreal 插件（AI Copilot）**：C++ + Slate UI 实现，提供工具栏入口、代码/蓝图/资源搜索、代码/蓝图生成功能；通过 HTTP API 与 AI Server 交互。插件同时具备脚本注入、Blueprint 解析、美术资源定位等能力。
- **AI Server**：基于 FastAPI/uvicorn，承载 Agent Orchestrator；每次请求由 Planner、Code、Asset、Doc 等 Agent 协同处理，整合 RAG 检索、LLM 生成与资源定位。Agent 系统采用 LangGraph/CrewAI/AutoGen 模式，多 Agent 协作完成复杂需求。
- **RAG Knowledge Base**：扫描 UE 项目（C++、Blueprint、资产、文档），通过 tree-sitter + custom parser 做 chunk 切片；数据经过 Embedding（BGE / Sentence Transformers），落入 Milvus / FAISS，供 vector 检索调用。
- **LLM Runtime & LLMOps**：集成 vLLM + FastAPI（兼容 OpenAI 接口），支持 Deepseek、Qwen、CodeLLM；提供 LoRA 微调机制，贴近 UE 代码风格。部署在本地 GPU/CPU 环境，兼容 DevOps 与 CI。
- **Vector Database**：Milvus / FAISS 搭配 metadata store（项目路径、类型、蓝图 ID）；支持数据库索引更新、版本控制与增量同步。

## 2. 系统细节

### 2.1 系统 1：游戏项目知识库

#### 目标
- 解决大型 Unreal 项目中“代码/资产/文档散落”问题。
- 让开发者通过自然语言查询“角色移动在哪个类”“某贴图在哪个关卡被引用”。

#### 数据管道
1. 代码扫描：`ue_project/Source` + `ue_project/Plugins`
2. Blueprint 解析：借助 `uecc`/`ue4assetparser` 抽取节点、变量与注释
3. Chunk 切分：按函数/蓝图子图/资产 metadata 切片
4. Embedding：BGE / Mistral / Sentence Transformers 处理文本
5. 写入 Vector DB（Milvus/FAISS）并附带 metadata（文件路径、类型、标签）
6. 检索 API：支持关键词+上下文+意图过滤

#### 技术栈
- Python 3.11、tree-sitter、unrealpak
- Embedding：BGE、text2vec、Normalizer
- 存储：Milvus v2.x + MinIO / PostgreSQL 用于 metadata
- 结果追踪：XFormers + Weaviate-like metadata

### 2.2 系统 2：Unreal Editor AI Copilot

#### 功能模块
- **AI 代码生成器**：输入要求（例如“创建冲刺技能”）→ Planner 产出步骤 → Code Agent 生成 C++/Header + 注册组件 + 编译脚本引用
- **蓝图解释器**：解析蓝图结构图，借助 RAG 返回节点功能说明 + 数据流
- **资源搜索助手**：结合 Vector DB 与资产索引，返回粒子特效/材质/音效路径
- **UI 入口**：Slate Panel、工具栏按钮、结果列表、右键菜单（“查资源”“解释节点”）

#### 插件架构
- Module：`AI_Copilot`（Editor Module）
- API：通过 Unreal HTTPClient 向 AI Server 发起带上下文请求（项目路径、类名、蓝图 ID）
- 本地缓存：保存最近查询 + 插件设置（模型偏好、上下文范围）

### 2.3 系统 3：AI 开发 Agent

#### 多 Agent 流程示例
```
用户“实现一个敌人 AI”
 ↓
Planner Agent：理解需求，拆解任务
 ↓
Code Agent：操作知识库 + LLM 生成 C++/Blueprint
 ↓
Asset Agent：在资源库/Vector DB 里找粒子/音效
 ↓
Doc Agent：生成设计文档、行为树说明
```

#### 技术栈
- LangGraph / AutoGen controller
- Agent Registry：定义 `Planner`, `Code`, `Asset`, `Doc` 的输入/输出 contract
- Task Queue：FastAPI + Redis + Celery（可选）
- Logging：OpenTelemetry + S3

### 2.4 系统 4：LLMOps

#### 职能
- 承载 Deepseek、Qwen、Code LLM 本地推理
- 提供 OpenAI API 兼容入口（`/v1/completions`, `/v1/chat/completions`）
- 管理 LoRA 微调任务（输入：UE 项目源码；输出：LoRA checkpoint）
- 监控模型性能、调用量、成本指标

#### 技术栈
- vLLM + FastAPI
- LoRA Toolkit（PEFT 或 LoRA Trainers）
- Model Registry（指向 `.bin` / `.safetensors`）
- Metrics：Prometheus + Grafana

## 3. 数据流与落地场景

1. UE 插件请求“角色冲刺逻辑”→ HTTP POST 至 AI Server
2. AI Server Agent 先在 RAG KB 里检索相关代码/蓝图/资产
3. Code Agent 调用 LLM Runtime 生成新的 C++ 文件并返回路径
4. Plugin 接收响应，创建文件/更新项目并提示用户
5. Agent 同步到 Vector DB 更新 chunk（新文件、注释）
6. LLMOps 监控 API 负载，自动横向扩展

## 4. 里程碑与落地指标
- **MVP 1**：搭建 RAG pipeline + Vector 知识检索，支撑自然语言问答
- **MVP 2**：AI Copilot 插件调用接口，生成一小段 C++ + Blueprint 解析
- **MVP 3**：Agent Orchestrator 完成 Code/Asset/Doc 链路
- **MVP 4**：LLMOps 支持 LoRA 入口与 OpenAI API 兼容

## 5. 未来扩展方向
- **AI NPC 系统**：Behavior Tree + LLM 生成 NPC 对话/任务
- **实时协同**：多人编辑 & 插件共享 prompts
- **安全策略**：访问控制、审计、模型防注入
