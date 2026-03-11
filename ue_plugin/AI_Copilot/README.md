# AI Copilot Unreal Plugin

## 目录结构
- `AI_Copilot.uplugin`：插件描述
- `Source/AI_Copilot`：C++ 模块代码
- `Source/AI_Copilot/Public`：对外接口
- `Source/AI_Copilot/Private`：Slate 面板与 HTTP helper

## 模块说明
- `AI_Copilot.Build.cs`：声明依赖模块（Slate、Http 等）
- `AI_Copilot.cpp/h`：模块生命周期，当前仅输出 log（可扩展为注册 toolbar、Slate UI）
- `CopilotPanel`：Slate 面板 placeholder，可在未来加入 Slate 构建逻辑
- `CopilotHttpClient`：负责向 `api/copilot/*` 发起请求

## 开发指导
1. 生成项目后，在 editor 中启用插件。
2. 模块启动会显示一条 dialog，用于确认加载成功。
3. 以 `FCopilotHttpClient::PostRequest` 为基础添加实际 prompt 请求逻辑。
