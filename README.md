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

项目基于 **C++17 / Qt** 开发，在保证低延迟与高并发处理能力的同时，提供直观、稳定且支持深度自定义的可视化管理界面。

---

## 🖼️ 界面预览

<details open>
<summary><b>📂 点击展开 / 折叠 界面预览截图</b></summary>
<br>

### 1. Classic 经典模式

<div align="center">

| 经典主界面 | 术语表侧滑与历史 |
| :---: | :---: |
| <img src="docs/ui_classic_new.png" width="400"> | <img src="docs/ui_classic_old.png" width="400"> |

<p align="center">
<em>固定像素布局 · 可折叠透明度抽屉 · 侧滑术语表 · 快速配置切换</em>
</p>

</div>

---

### 2. Modern 流光模式

<div align="center">

<img src="docs/ui_modern.png" width="600">

<p align="center">
<em>毛玻璃设计 (Glassmorphism) · 动态渐变边框 · 色相与流光浓度调节 · 子窗口样式同步</em>
</p>

| 毛玻璃模式 (Frosted) | 经典发光模式 (Legacy) |
| :---: | :---: |
| <img src="docs/流光1.png" width="400"> | <img src="docs/流光2.png" width="400"> |

</div>

---

### 3. ⚙️ 高级设置面板 (Advanced Settings)

<div align="center">

| 网络请求策略 (Tab 1) | 配置智能劫持 (Tab 2) |
| :---: | :---: |
| <img src="docs/advance1.png" width="400"> | <img src="docs/advance2.png" width="400"> |

<p align="center">
<em>标准 / 保守 / 快速 / 自定义策略调节 · 游戏源/目标语言与 7 大文本框架拦截精细控制</em>
</p>

</div>

---

### 4. 🔍 游戏环境检测中心 (Environment Check)

<div align="center">

| 环境依赖扫描结果 | 帮助指南与排错 |
| :---: | :---: |
| <img src="docs/auto1.png" width="400"> | <img src="docs/auto2.png" width="400"> |

<p align="center">
<em>Mono / IL2CPP 架构识别 · Unity 版本解析 · XUnity 与字体资产诊断 · 一键下载部署</em>
</p>

</div>

</details>

---

## 🔄 与早期 C++ 版本对比

<div align="center">

| 功能维度 | 早期版本 | 当前版本 |
| :---: | :---: | :---: |
| **UI 架构** | 单一主窗口 | **双模式 UI（Classic 经典 / Modern 流光）** |
| **视觉风格** | 固定主题 | **经典双色 + 毛玻璃 (Frosted) / 经典发光 (Legacy)** |
| **透明度控制** | 不支持或全局混杂 | **双模式独立透明度记忆（经典可折叠面板 / 流光滑动调节）** |
| **游戏环境体检** | ❌ 无 | **✅ 架构检测 + 引擎版本解析 + 依赖检测 + 一键下载部署** |
| **高级设置面板** | ❌ 无 | **✅ 请求策略预设 + 语言端点定制 + 7 大文本框架深度接管** |
| **配置文件接管** | 仅修改端口 | **✅ 深度接管语言、端点、文本获取模式及文本渲染框架** |
| **多行打包翻译** | ❌ 仅逐行处理 | **✅ Batch 并发打包 + 路径安全校验 + 自动备份与还原** |
| **Token 统计** | 简单累加 | **✅ 读写锁线程安全 + 覆盖率追踪 + 输入/输出/附加计费项明细** |
| **上下文记忆** | 0~20 轮 | **✅ 0~100 轮动态上下文调节** |
| **术语维护** | 仅读取文件 | **✅ RAG 上下文注入 + 术语自动提取 + 独立语法高亮编辑器** |
| **HUD 状态窗** | ✅ 支持 | **⚠️ 该版本中暂时移除（经典模式入口已改为高级设置）** |

</div>

---

## 🛠️ 核心功能

### 🔍 1. 游戏环境检测与一键修复中心 (Environment Check)

* **自动化环境扫描**：
  * **架构与框架检测**：识别游戏运行后端（**Mono** 或 **IL2CPP**），并检测 BepInEx、MelonLoader 等前置插件框架。
  * **Unity 引擎版本获取**：解析 `UnityPlayer.dll` 或主程序信息，提取引擎核心版本号。
  * **翻译组件与字体排查**：检测 `XUnity.AutoTranslator` 插件本体、`_Substitutions.txt` 术语词库及中文字体资产配置，排查游戏内缺字乱码（□□□）隐患。
* **一键下载部署**：
  * 对缺失的 XUnity 核心及 BepInEx 框架提供一键下载与解压部署支持。
  * 内置 GitHub 加速镜像节点调度，降低网络连接失败概率。
* **内置使用帮助**：整合排错说明、常见报错速查与字体替换指引。

---

### ⚙️ 2. 高级设置与请求策略管理 (Advanced Settings)

* **网络与重试策略**：
  * 提供 `标准模式`（20次重试 / 10s超时）、`保守模式`（30次 / 15s）、`快速模式`（3次 / 5s）与 `自定义模式`。
  * 服务运行中修改即时生效，底层引擎在重试过程中支持动态继承新参数。
* **游戏配置 (INI) 深度定制**：
  * 支持配置游戏源语言（`FromLanguage`）与目标语言（`Language`）。
  * 支持切换翻译端点（`GoogleTranslate` / `CustomTranslate`）。
  * 支持开关 `TextGetterCompatibilityMode`（文本获取兼容模式）。
  * 支持精细控制 **7 大文本框架** 拦截状态（`IMGUI`, `UGUI`, `UIElements`, `NGUI`, `TextMeshPro`, `TextMesh`, `FairyGUI`）。
  * 退出程序时根据备份文件精准还原配置。

---

### 🎨 3. 双模式交互界面

* **Classic 经典模式**：
  * **可折叠透明度抽屉**：点击箭头按钮展开/收起透明度调节面板（设有安全底线阈值，防止窗口因透明度过低而不可见）。
  * **平滑切换动效**：主题与语言切换具备过渡动画，减少突变感。
  * 支持窗口自由缩放，兼顾低硬件占用与稳定性。
* **Modern 流光模式**：
  * 提供 **毛玻璃效果 (Frosted)** 与 **经典发光 (Legacy)** 两种渲染模式。
  * 支持全局 **色相偏移 (Hue Shift)** 与 **流光浓度 (Tint Intensity)** 自定义调节。
  * 子窗口（环境检测、高级设置、术语表抽屉）与主窗口保持风格与配色同步。

> **注**：原 HUD 迷你悬浮窗在该版本中暂时移除，以便后续进行架构重构；经典模式原按钮位置现已分配给「高级设置」功能。

---

### 🧠 4. 翻译处理与标签保护

* **多行并发打包 (Batch Mode)**：
  接管游戏批处理参数，提升大文本量 RPG 和界面密集型游戏的翻译吞吐效率。
* **防上下文污染 (Anti-Bleed)**：
  保护 `[LF]` 与 `<T_0>` 等占位符，通过提示词约束单行独立性，降低长句幻觉与串行补全概率。
* **标签保护与修复**：
  逐行识别并保留游戏内嵌标签（如 `<color>`、`<b>`、`<ruby>` 等），修复缺失或异常闭合标签，支持 Unity `<rotate>` 竖排文本拆分还原。
* **术语自进化 (RAG)**：
  翻译时自动关联术语库，并可从模型返回结果中提取未收录的新专有名词追加写入本地词库。

---

### 📊 5. Token 统计与监测

* **线程安全数据统计**：采用读写锁设计，确保高并发请求下数据更新稳定。
* **多模型格式兼容**：支持解析 OpenAI、DeepSeek、Gemini（`usageMetadata`）及 Ollama 等格式的 Usage 字段。
* **覆盖率与明细展示**：
  悬浮提示中展示输入 Prompt、输出 Completion、供应商总计明细，并统计返回了 Usage 数据的响应比例，避免虚构估算。

---

## 🚀 快速开始

### 1. 标准模式（逐行翻译）

1. 启动程序，配置 API 密钥（API Key）、接口地址与模型名称。
2. 点击 **测试配置** 验证网络连通性。
3. 测试通过后点击 **启动服务**。
4. 在游戏目录 `AutoTranslator/Config.ini` 中配置：
   ```ini
   [Service]
   Endpoint=CustomTranslate
   
   [Custom]
   Url=http://localhost:6800   # 端口需与 GUI 设置保持一致
   ```

---

### 2. 打包并发模式（Batch 模式 · 推荐）

1. 在界面中选择有效的术语表（`_Substitutions.txt`）路径，程序将依此自动定位游戏配置文件。
2. 勾选 **📦 多行模式 (Batch)**。
3. （可选）点击 **高级设置** 定制语言、端点或文本框架拦截开关。
4. 点击 **启动服务**（程序将自动备份原始配置并写入中转参数）。
5. 启动游戏进行翻译；退出程序后原配置将自动还原。

---

### 3. 环境检测与依赖安装 (Env Check)

1. 点击主界面上的 **🔍 环境检测** 按钮。
2. 选择游戏主程序所在根目录。
3. 查看 Mono / IL2CPP 架构识别结果；如缺少 XUnity 或前置框架，点击对应项的 **部署** 即可自动下载并安装。

---

## 📂 代码结构

```text
src/
├── main.cpp                     # 程序入口、窗口管理与切换过渡逻辑
├── MainWindow.cpp/h             # Classic 模式主界面及核心交互
├── ModernWindow.cpp/h           # Modern 模式主界面、无边框绘制与事件处理
├── ModernUI.h                   # 毛玻璃渲染核心与组件样式
├── AdvancedSettings.cpp/h       # 高级设置面板（网络策略、INI 配置劫持）
├── EnvScanWindow.cpp/h          # 经典模式环境检测与依赖诊断中心
├── ModernEnvScanWindow.cpp/h    # 流光模式环境检测与依赖诊断中心
├── DependencyInstaller.cpp/h    # 依赖项下载与静默解压部署模块
├── TranslationServer.cpp/h      # HTTP 转发服务器与标签修复核心
├── XuaConfigHijacker.h          # 游戏 INI 智能劫持、备份与还原组件
├── ConfigManager.cpp/h          # 配置文件读写与多模式参数持久化管理
├── GlossaryManager.h            # 术语表读写与 RAG 注入模块
├── RegexManager.h               # 正则前置/后置文本处理
├── TokenManager.cpp/h           # 线程安全 Token 统计与覆盖率分析器
├── HudWindow.cpp/h              # HUD 悬浮窗组件（该版本暂时停用）
└── LoadingOverlay.h             # 动态加载遮罩控件
```

---

## 🛠️ 编译指南

### 环境要求
- **编译器**：支持 C++17 的编译器（MSVC 2019+、MinGW-w64 8.1+、Clang 11+）
- **开发框架**：Qt 6.2.0 或更高版本（需包含 `Qt Network`、`Qt Widgets` 模块）
- **构建工具**：CMake 3.16 或更高版本

### 构建步骤
```bash
git clone https://github.com/your-repo/XUnity-LLM-Translator-GUI.git
cd XUnity-LLM-Translator-GUI
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=C:/Qt/6.5.0/msvc2019_64   # 替换为实际 Qt 路径
cmake --build . --config Release
```

### 运行时依赖打包
```bash
windeployqt --release --compiler-runtime XUnity-LLM-Translator-GUI.exe
```

---

## 📝 开源许可

本项目基于 **MIT** 许可证开源。您可以自由使用、修改和分发，但须在衍生版本中保留原作者版权声明。

---

> 📖 English Documentation: [README_US.md](README_US.md)