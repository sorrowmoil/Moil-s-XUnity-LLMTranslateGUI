# XUnity LLM Translator GUI

<div align="center">

<h1>
  <a href="README_US.md">English</a> | <a href="README.md">中文</a>
</h1>

</div>

<div align="center">

<img src="https://img.shields.io/badge/license-MIT-green" height="25">  
<img src="https://img.shields.io/badge/Qt-6.x-blue" height="25">  
<img src="https://img.shields.io/badge/C++-17-orange" height="25">
<img src="https://img.shields.io/badge/Platform-Windows-lightgrey" height="25">

</div>

---

## 📖 简介

**XUnity LLM Translator GUI** 是一款专为 Unity 游戏设计的本地翻译中转工具。它作为一个轻量级的本地 HTTP 转发服务器，用于将 **XUnity.AutoTranslator** 的请求桥接至各类大语言模型（如 Grok、DeepSeek、OpenAI、Gemini、Ollama 等）。

项目基于 **C++17 / Qt** 编写，旨在在保证**低延迟**与**高并发处理能力**的同时，提供一个稳定、直观的可视化配置管理界面。

---

## 🖼️ 界面预览

### Classic 模式（新旧对比）

| 早期版本 | 当前版本 |
| :--: | :--: |
| <img src="docs/ui_classic_old.png"> | <img src="docs/ui_classic_new.png"> |

<p align="center">
<em>固定像素布局 · 术语表侧滑窗口 · 新增功能按钮</em>
</p>

---

### Modern 流光模式

<p align="center">
<img src="docs/ui_modern.png">
</p>

<p align="center">
<em>Glassmorphism UI · 动态渐变描边 · 侧滑术语表 · 实时透明度调节</em>
</p>

---

## 🔄 与早期 C++ 版本对比

| 功能维度        | 早期 C++ 版本 |             当前版本             |
| :---------- | :-------: | :--------------------------: |
| UI 架构       |   单一主窗口   | **双模式 UI（Classic / Modern）** |
| 界面风格        |    固定主题   |  **经典 + 流光 (Glassmorphism)** |
| 内置术语编辑      |    ❌ 无    |        **✅ 内置侧滑术语编辑器**       |
| 多行打包翻译      |   ❌ 仅逐行   |      **✅ Batch 模式并发打包**      |
| Config 自动接管 |   ❌ 手动配置  | **✅ 一定程度自动化且自动备份** |
| 上下文污染防护     |   基础占位保护  |    **✅ Anti-Bleed 多行隔离机制**   |
| 术语系统        |  RAG 自动补充 |        **RAG + 可视化编辑**       |
| UI 动画系统     |   基础 Qt   |     **Modern 动效 + 毛玻璃渲染**    |
| HUD 状态窗     |    ✅ 支持   |        ✅ 支持 + Token 统计       |
| API Key 轮询  |     ✅     |               ✅              |
| 热重载         |     ✅     |               ✅              |
| 并发线程池       |   未仔细定义  |        **64~256**        |
| 错误提示        |    基础映射   |       **更完整 HTTP 错误提示**      |

---

## 🛠️ 核心功能

### 🎨 双模式界面

* **Classic 模式**：轻量稳定，适合较老设备，资源占用更低。
* **Modern 流光模式**：采用 Glassmorphism 毛玻璃设计，支持动态渐变描边、实时透明度调节，视觉体验更现代。
* **内置术语编辑器**：侧滑窗口设计，支持原文/译文的语法高亮，方便快速维护术语表。

---

### 🧠 翻译逻辑处理

* **多行打包路由 (Batch Mode)**
  自动接管 `Config.ini` 实现并发打包翻译，大幅提升文本密集型游戏的翻译吞吐量。
  退出程序时自动恢复原始配置，无需手动干预。

* **防上下文污染 (Anti-Bleed)**
  在请求发送前自动保护 `[LF]` 与 `<T_0>` 等游戏内特殊标记，并通过提示词要求模型将每行文本视为独立片段，有效避免模型“脑补”上下文导致的逻辑混乱。

* **术语自进化 (RAG)**
  翻译时自动读取术语表作为上下文参考，并根据模型返回结果智能提取未收录的新名词，补充到术语文件中，实现术语库的持续进化。

---

### 🚀 并发与服务控制

* **异步线程池**：基于 `httplib` 实现高性能线程池，支持 64~256 并发请求，满足高负载场景。
* **API Key 自动轮询**：支持填写多个以逗号分隔的 API Key，系统自动负载均衡，避免单一 Key 触发限流。
* **动态热重载**：运行中修改模型名称、API Key、系统提示词或采样温度等配置，下一次请求即自动生效，无需中断服务。

---

### 🛡️ 状态监测与容错

* **HUD 悬浮窗**：可切换至迷你状态窗，通过三色呼吸灯（绿/青/红）直观显示当前工作状态，并实时统计 Token 消耗。
* **人性化错误映射**：将常见的 HTTP 状态码（401、429、500 等）及网络超时转化为清晰的中文操作建议，帮助用户快速定位问题。
* **强制超时保护**：内置 10~40 秒的超时机制，防止因 API 响应缓慢导致游戏逻辑长时间卡死。

---

## 🚀 快速开始

### 标准模式（逐行处理）

1. 启动程序，填写 API Key 和模型信息。
2. 点击 **测试配置** 验证连通性。
3. 测试通过后点击 **启动服务**。
4. 手动编辑游戏目录下的 `AutoTranslator/Config.ini`：
   ```ini
   [Service]
   Endpoint=CustomTranslate
   
   [Custom]
   Url=http://localhost:6800   # 端口需与 GUI 中设置保持一致
   ```

---

### 📦 打包模式（多行并发，推荐用于文本密集型游戏）

1. 在主界面勾选 **📦 多行模式 (Batching)**。
2. 确保已正确选择术语表（`.txt`）路径，程序将依赖此路径自动定位游戏的 `Config.ini`。
3. 点击 **启动服务**（程序将自动修改并接管游戏配置）。
4. 启动游戏。翻译过程中将自动应用打包模式，**自动备份当前配置文件**。

---

## 📂 代码结构

```text
src/
├── main.cpp                     # 应用入口，UI 模式切换与过渡动画
├── MainWindow.cpp/h             # Classic 模式主界面及业务逻辑
├── ModernWindow.cpp/h           # Modern 模式主界面
├── TranslationServer.cpp/h      # HTTP 服务器、API 交互与重试逻辑
├── XuaConfigHijacker.h          # 游戏配置自动修改与还原组件
├── GlossaryManager.h            # 术语表读写与 RAG 注入逻辑
├── RegexManager.h               # 文本预处理与后处理正则
├── HudWindow.cpp/h              # HUD 悬浮状态窗
├── ModernUI.h                   # 流光模式组件库（内置编辑器、渲染代理）
├── ConfigManager.cpp/h          # 配置文件（config.ini）序列化读写
└── TokenManager.cpp/h           # Token 统计与管理
```

---

## 🛠️ 编译指南

### 环境要求
- 支持 C++17 的编译器（MSVC 2019+、MinGW 8.1+、Clang 11+）
- Qt 6.2.0 或更高版本（包含 Qt Network、Qt Widgets 等模块）
- CMake 3.16 或更高版本

### 构建步骤
```bash
git clone https://github.com/your-repo/XUnity-LLM-Translator-GUI.git
cd XUnity-LLM-Translator-GUI
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=C:/Qt/6.5.0/msvc2019_64   # 替换为你的 Qt 路径
cmake --build . --config Release
```

### 注意事项
- 若使用 MinGW，请确保 `CMAKE_PREFIX_PATH` 指向正确的 Qt 安装目录。
- 编译后的可执行文件位于 `build/Release/` 目录下。

---

> **致二次开发者**：
> 为保证 UI 元素的完美对齐与最佳视觉体验，若您需要进行界面上的二次开发，请尽量将主窗口的像素大小硬性限制在 `500 x 832` 左右。

## 📦 部署与打包

### 依赖收集
使用 Qt 自带的 `windeployqt` 工具收集运行时依赖：
```bash
windeployqt --release --compiler-runtime XUnity-LLM-Translator-GUI.exe
```

### 单文件封装
如需生成单文件可执行程序，可使用 **Enigma Virtual Box** 或 **BoxedApp Packer** 将依赖库打包进主程序。注意保留必要的 Qt 插件目录（如 `platforms`、`styles` 等）。

---

## 📝 License

本项目基于 **MIT** 许可证开源。您可以自由使用、修改及分发，但需保留原作者版权声明。

---

> 📖 English version: [README_US.md](README_US.md)
