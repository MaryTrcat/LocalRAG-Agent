# LocalRAG Agent

LocalRAG Agent 是一个以本地知识库为基础的问答项目，包含 React 网页版、Qt 桌面版和 C++ 命令行程序。资料、文本切片、会话消息和检索记录保存在本机 SQLite 数据库中；配置真实的 OpenAI 兼容 API 后，可由模型根据检索到的资料回答问题。

## 当前功能

- 网页版：搜索和查看浏览器中的历史对话；从设置导入 Markdown、TXT 或 DOCX；查看资料内容、单篇删除或清空本地库；设置 API 地址、模型和 API Key。
- 桌面版：导入 Markdown 或 TXT、查看知识库统计、提出问题并查看检索来源；设置 API、检索数量和文本切片参数。
- 命令行：初始化数据库、导入和删除文档、查看文档列表和统计、提问、清空知识库。
- 后端：C++17、SQLite、文本切片、RAG 提示词构造、检索和 OpenAI 兼容 Chat Completions 接口。

## 网页版启动

Windows 上推荐使用项目根目录的启动脚本：

```powershell
.\run-localrag.bat
```

脚本会检查前端依赖和本地 `localrag.exe`，然后启动网页服务。浏览器打开：

```text
http://127.0.0.1:4190/
```

Node.js 18 或更高版本及 npm 必须可用。首次运行若缺少前端依赖，脚本会执行 `npm ci`。请保持启动窗口开启；按 `Ctrl+C` 停止服务。

默认仅允许本机访问。如果要从同一可信局域网的其他设备访问，运行：

```powershell
.\run-localrag.bat -LanAccess
```

然后使用窗口中显示的 `LocalRAG LAN URL`。局域网模式会开放本机资料库 API，当前没有访问认证；不要在公共或不可信网络中使用。

## 网页 API 设置

打开网页左下角的“设置”，在“API 连接”中填写兼容 OpenAI Chat Completions 的 Base URL、模型名称和 API Key，然后保存。默认值为 DeepSeek：

- Base URL：`https://api.deepseek.com/v1`
- 模型：`deepseek-chat`

网页会将设置（包括 API Key）保存到当前浏览器的 `localStorage`，不会写入项目配置文件。请不要在公用浏览器配置密钥。若未填写 API Key，真实模型问答接口会提示先配置密钥。

## 桌面版

如果已构建程序，可从项目根目录运行：

```powershell
.\build-desktop-final\localrag_desktop.exe
```

桌面版设置保存在 `config/localrag.json`；API Key 不写入该文件，而是从设置中指定的环境变量读取。默认变量名为 `LOCALRAG_API_KEY`。例如：

```powershell
$env:LOCALRAG_API_KEY = "你的 API Key"
.\build-desktop-final\localrag_desktop.exe
```

## 命令行

命令行程序使用 `data/localrag.db` 作为默认数据库。以下命令均从项目根目录运行：

```powershell
.\build-desktop-final\localrag.exe init
.\build-desktop-final\localrag.exe import data\sample.md
.\build-desktop-final\localrag.exe list-docs
.\build-desktop-final\localrag.exe stats
.\build-desktop-final\localrag.exe ask "RAG 有什么作用？"
.\build-desktop-final\localrag.exe delete 1
.\build-desktop-final\localrag.exe clear
```

配置真实模型时，在同一个 PowerShell 窗口设置环境变量：

```powershell
$env:LOCALRAG_API_KEY = "你的 API Key"
$env:LOCALRAG_BASE_URL = "https://api.deepseek.com/v1"
$env:LOCALRAG_CHAT_MODEL = "deepseek-chat"
.\build-desktop-final\localrag.exe ask "RAG 有什么作用？"
```

## 数据与当前限制

- SQLite 数据库默认位于 `data/localrag.db`；网页版导入的原始资料副本位于 `data/imported/`。
- 网页版的历史对话和 API 设置保存在当前浏览器中；数据库另行保存问答消息和检索记录，两者不是跨浏览器同步的同一份历史列表。
- 当前向量化使用项目内的简化本地 Fake Embedding，向量索引在内存中构建；它不是语义 Embedding 服务，也没有集成 FAISS 或 MySQL。
- 网页版支持 `.md`、`.txt`、`.docx` 导入；桌面版和 C++ 命令行目前仅支持 `.md`、`.txt`。
- 没有配置 API Key 时，桌面版和命令行会使用模拟回答；要获得真实模型回答，需配置有效的 OpenAI 兼容 API。

## 构建与测试

项目使用 CMake 构建 C++ 程序，依赖 Qt 6 和 SQLite。若已配置 `build-desktop-final` 构建目录，可运行：

```powershell
cmake --build build-desktop-final --config Release --parallel 4
ctest --test-dir build-desktop-final -C Release --output-on-failure
```

测试覆盖文本切片、提示词构造、检索排序、问答记录、文档导入、OpenAI 兼容响应解析和 SQLite 持久化。

前端生产构建：

```powershell
cd frontend
npm ci
npm run build
```

浏览器冒烟测试位于 `frontend/smoke_test.py`，需要 Playwright Python 包和 Chromium，并要求本地网页服务运行在 `http://127.0.0.1:4186`（或通过 `LOCALRAG_TEST_URL` 指定测试地址）。

## 主要目录

```text
src/                 C++ 核心、CLI、Qt 桌面程序
include/localrag/    C++ 公共接口
tests/               C++ 单元测试
frontend/            React + TypeScript + Vite 网页
data/                SQLite 数据库、示例与导入资料
config/              桌面版配置
run-localrag.bat     Windows 网页启动入口
```
