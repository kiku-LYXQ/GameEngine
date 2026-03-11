# Game Project Dataset

## 目标
为 AI GameDev Platform 提供可以作为 RAG 输入的 UE 项目数据，包括源代码、Blueprint 定义、资产 metadata 与设计文档。

## 数据采集思路
1. **源码**：从 `Source/` 目录抓取 `.cpp`, `.h`，解析函数、注释并补全上下文。
2. **Blueprint**：通过 Unreal JSON / UAsset 解析器导出事件图、节点说明、输入输出数据。
3. **资产**：提取 Content 文件夹中 `.uasset`、`.umap` 的 metadata（路径、种类、用途）。
4. **文档**：聚合设计文档、GDD、wiki 文章，统一转成 Markdown/Plain text。

## 处理流程
- Chunk 切分（函数级、节点组、资源描述）
- Embedding（BGE/text2vec），同时记录 `metadata`（`path`, `entity_type`, `tags`, `line_range`, `hash`）
- 存储至 Milvus/FAISS，便于后续 RAG 检索

## 数据脚本
- `scripts/collect_assets.py`：遍历项目目录，生成 manifest.json（path/size/timestamp），作为采集阶段快照。
- `scripts/chunk_pipe.py`：对 Markdown 文档进行 chunk 切片并生成 placeholder embedding + metadata，可直接向 Vector Store 写入。

## 预留内容
- 增加 `game_project/sample_data/` 作为灰度测试样本
- future: Blueprint parser、asset metadata extractor
