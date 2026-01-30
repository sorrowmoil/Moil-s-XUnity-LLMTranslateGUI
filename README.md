# XUnity LLM Translator GUI

<div align="center">

<h1>
  <a href="README_US.md">English</a> | <a href="README.md">中文</a>
</h1>

</div>

<div align="center">

<img src="https://img.shields.io/badge/license-MIT-green" height="40">  
<img src="https://img.shields.io/badge/python-3.9+-blue" height="40">  
<img src="https://img.shields.io/badge/C++-17-orange" height="40">

</div>


---

## 🇨🇳 中文版本

### 简介
**XUnity LLM Translator GUI** 是一个高性能的本地 HTTP 转发服务器，用于连接 **XUnity.AutoTranslator**（Unity 游戏自动翻译插件）与大语言模型（OpenAI, Gemini, Claude, DeepSeek 等）。

- **源码**：Python 和 C++ 实现均在 `main` 分支提供  
- **可执行文件**：编译好的 `.exe` 文件在 [Releases](../../releases) 提供  
- **版本说明**：  
  - **Fox** → 原始版本（动画流畅，UI稳定）  
  - **Moil** → 增强版（支持多语言、术语表、主题切换等高级功能）  

---

### 🔄 版本历史

#### Python 版本
- **Fox 版**：稳定基础实现，动画流畅  
- **Moil 版**：功能增强，但 UI 有取舍  

#### C++ 版本
- **完全重构**：超低延迟、高并发的现代实现  

---

### ✨ 核心特性

#### 🚀 通用功能
- 高性能并发处理  
- 多 API Key 轮询  
- 现代化 UI + 实时日志  
- 自动保存配置  

#### 🎯 Moil Python 终极版
- 🌍 中英双语界面  
- 🎨 主题切换 (darkly/flatly)  
- 📚 智能术语表，自动提取新词  
- ⚡ 配置导入/导出  
- 🔧 改进错误处理，多密钥支持  

#### ⚡ C++ 版本
- 原生性能，超低延迟  
- 高级线程池并发  
- 正则预处理/后处理支持  
- RAG 上下文注入  
- 智能采样算法  

---

### ⚠️ 版本对比

| 功能         | Moil Python | Fox Python | C++ |
|--------------|-------------|------------|-----|
| 多语言界面   | ✅          | ❌         | ✅  |
| 主题切换     | ✅          | ❌         | ✅  |
| 术语表系统   | ✅          | ❌         | ✅  |
| 动画流畅度   | ⚠️ 稍差     | ✅ 流畅    | ✅  |
| UI 布局      | ⚠️ 部分问题 | ✅ 稳定    | ✅  |
| 性能表现     | 良好        | 良好       | 优秀 |
| 部署复杂度   | 简单        | 简单       | 需编译 |

---

### 🚀 快速开始

#### 方式一：直接运行可执行文件（推荐）
1. 从 [Releases](../../releases) 下载最新 `.exe`  
2. 双击运行  
3. 填写 API 地址、密钥和端口，点击 **启动服务**

#### 方式二：源码运行（开发者）
```bash
pip install ttkbootstrap openai requests
python XUnity-Moil的LLMTranslateGUI.py
```

---

### 🧩 文件结构

#### C++ 版本 (`src/`)
```text
src/
├── ConfigManager.cpp / .h         # 配置管理
├── GlossaryManager.h              # 术语表管理
├── htestlib.h                     # 工具头文件
├── json.hpp                       # JSON 解析器（nlohmann/json）
├── main.cpp                       # 程序入口
├── MainWindow.cpp / .h           # GUI 逻辑
├── RegexManager.cpp / .h         # 正则预处理/后处理
├── translate.ico                  # 应用图标
├── TranslationServer.h           # HTTP 服务器接口
├── HudWindow.cpp/h               # HUD呼吸灯模块
```

#### Python 版本 (`Python/`)
```text
Python/
├── XUnity-Moli@LLMTtranslatedGUI.py   # 主 GUI 脚本（Moil 版本）
├── moli.ico                           # 应用图标
```

---

### 🛠️ 使用方法

#### 配置 XUnity.AutoTranslator
编辑 `AutoTranslator/Config.ini`：
```ini
[Service]
Endpoint=http://localhost:6800
```

#### 术语表功能（Moil 和 C++ 版本）
- 启用 **自进化** 选项  
- 选择 `_Substitutions.txt` 文件  
- 工具会参考现有术语并自动学习新词  

---

### ⚙️ 编译指南（开发者）

#### C++ 版本
- 环境要求：CMake 3.16+, Qt 6.x, C++17 编译器（MSVC/MinGW）  
- 构建命令：
  ```bash
  mkdir build && cd build
  cmake ..
  cmake --build . --config Release
  ```
  > 💡 对于 Visual Studio 等多配置生成器，需要 `--config Release` 来构建优化版本。在 Linux/macOS 上通常不需要此标志。

#### Python 版本
- **Moil**：推荐使用（功能丰富）  
- **Fox**：备选方案（动画流畅，布局稳定）  
- 通过 pip 安装依赖，无需编译  

---

### 🎯 开发路线图 & 待办事项

<details>
<summary><strong>🚀 性能优化</strong></summary>

- [x] ~~优化翻译速度~~ - **Moil版已完成**: 多线程HTTP服务器，上下文缓存，API密钥轮询
- [x] ~~加入并发支持~~ - **Moil版已完成**: ThreadingHTTPServer + 线程池支持
</details>

<details>
<summary><strong>🌐 语言支持</strong></summary>

- [x] ~~加入对英语的支持~~ - **Moil版已完成**: 完整的中英双语界面，实时语言切换
</details>

<details>
<summary><strong>🛠️ 代码重构</strong></summary>

- [x] ~~使用其他语言重构~~ - **已完成**: C++版本已重构完成，提供更高性能
- [ ] 进一步代码优化和模块化
</details>

<details>
<summary><strong>✨ Moil版本新增功能</strong></summary>

- [x] **多语言界面**: 完整的中英文双语支持
- [x] **主题系统**: 亮色/暗色主题切换
- [x] **智能术语表**: 自进化术语管理系统
- [x] **增强配置管理**: 配置导入导出功能
- [x] **工具提示系统**: 详细的控件功能说明
- [x] **平滑动画**: 淡入淡出效果
- [x] **上下文菜单**: 日志区域右键功能
- [x] **多API密钥支持**: 自动轮询多个密钥
</details>

<details>
<summary><strong>🔮 未来增强计划</strong></summary>

- [ ] 更多语言界面支持
- [x] ~~术语表管理界面~~ -**已完成**: C++版本已重构完成，支持**自动捕获**术语
</details>

---

### 📝 许可证
MIT 许可证。可自由 fork 和修改。

### 💡 打包说明
- Python：**PyInstaller**  
- C++：**Enigma Virtual Box** / **windeployqt**

---

> 📖 **English version available**: [README_US.md](README_US.md)
