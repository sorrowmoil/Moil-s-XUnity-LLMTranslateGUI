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

## 📖 Introduction

**XUnity LLM Translator GUI** is a local translation relay tool designed specifically for Unity games. It acts as a lightweight local HTTP forwarding server that bridges requests from **XUnity.AutoTranslator** to various large language models (such as Grok, DeepSeek, OpenAI, Gemini, Ollama, etc.).

Built with **C++17 / Qt**, the project aims to deliver a stable and intuitive graphical configuration interface while ensuring **low latency** and **high concurrency handling**.

---

## 🖼️ Interface Preview

### Classic Mode (Old vs. New)

| Earlier Version | Current Version |
| :--: | :--: |
| <img src="docs/ui_classic_old.png"> | <img src="docs/ui_classic_new.png"> |

<p align="center">
<em>Fixed pixel layout · Slide-out glossary panel · New function buttons</em>
</p>

---

### Modern "Streaming Light" Mode

<p align="center">
<img src="docs/ui_modern.png">
</p>

<p align="center">
<em>Glassmorphism UI · Dynamic gradient stroke · Slide-out glossary · Real-time transparency adjustment · Custom themes</em>
</p>

| Glassmorphism On | Glassmorphism Off |
| :--: | :--: |
| <img src="docs/流光1.png"> | <img src="docs/流光2.png"> |

---

## 🔄 Comparison with the Earlier C++ Version

| Feature Dimension        | Earlier C++ Version |             Current Version              |
| :----------------------- | :-----------------: | :--------------------------------------: |
| UI Architecture          |  Single main window | **Dual‑mode UI (Classic / Modern)**      |
| Visual Style             |    Fixed theme     | **Classic + Glassmorphism (Modern)**    |
| Built‑in Glossary Editor |        ❌ No       |      **✅ Built‑in slide‑out editor**     |
| Batch (Multi‑line) Translation |   ❌ Line‑by‑line only   |     **✅ Batch mode with concurrent packing** |
| Auto Config Hijacking    |   ❌ Manual setup   | **✅ Fully automatic with backup** |
| Context Bleed Prevention | Basic placeholder protection | **✅ Anti‑Bleed multi‑line isolation** |
| Glossary System          | RAG auto‑supplement |       **RAG + visual editing**          |
| UI Animation System      |     Basic Qt       | **Modern effects + glassmorphism rendering** |
| HUD Status Window        |     ✅ Supported    |         ✅ Supported + Token stats        |
| API Key Rotation         |         ✅          |                    ✅                    |
| Hot Reload               |         ✅          |                    ✅                    |
| Concurrency Thread Pool  |    Not well‑defined |            **64 ~ 256**                 |
| Error Prompts            |    Basic mapping   |      **More comprehensive HTTP error tips** |

---

## 🛠️ Core Features

### 🎨 Dual‑Mode Interface

* **Fully resizable window**: Both Classic and Modern modes support **free window scaling**, adapting flexibly to different monitor resolutions and user habits.
* **Classic Mode**: Lightweight and stable, ideal for older hardware with lower resource consumption.
* **Modern "Streaming Light" Mode**: Adopts a Glassmorphism design with a frosted glass effect. It supports dynamic gradient strokes, real‑time transparency adjustment, and now includes **custom theme functionality**, allowing you to freely mix glassmorphism intensity and color schemes for a more personalized, modern visual experience.
* **Built‑in Glossary Editor**: A slide‑out panel with syntax highlighting for source/translation text, making it easy to maintain your glossary quickly.

---

### 🧠 Translation Logic Processing

* **Batch Routing Mode**
  Automatically takes over `Config.ini` to enable concurrent packed translation, greatly improving translation throughput for text‑heavy games.
  The original configuration is automatically restored when you exit the program — no manual intervention needed.

* **Anti‑Bleed (Context Pollution Prevention)**
  Protects special in‑game markers such as `[LF]` and `<T_0>` before sending requests, and uses prompts to instruct the model to treat each line as an independent segment. This effectively prevents logical confusion caused by the model “hallucinating” context.

* **Self‑Evolving Glossary (RAG)**
  Automatically reads the glossary as contextual reference during translation, and intelligently extracts new, unrecorded terms from the model’s responses to supplement the glossary file, enabling continuous evolution of your termbase.

* **📝 Text Preprocessing & Formatting**
  - **Rich Text Preservation**: Automatically detects and protects in‑game rich text tags such as colors, bold, italic, etc., preventing tag loss or corruption during translation. This significantly reduces text fragmentation and improves translation completeness.
  - **Native Line Break Retention**: Strictly preserves original line breaks (e.g. `\n`), respecting the game’s native layout as much as possible, so that paragraphs in dialogs, menus, etc. match the original structure.

---

### 🚀 Concurrency & Service Control

* **Asynchronous Thread Pool**: High‑performance thread pool based on `httplib`, supporting 64–256 concurrent requests to handle heavy loads.
* **Automatic API Key Rotation**: Supports multiple comma‑separated API keys; the system automatically load‑balances among them to avoid rate limiting on a single key.
* **Dynamic Hot Reload**: Changes to model name, API key, system prompt, or temperature take effect on the next request without restarting the service.
* **⚙️ Custom Preset Management**: Allows you to add API endpoint presets for other service providers or relay stations. Presets are automatically persisted, making it easy to switch between different service configurations with a single click — no repeated typing required.

---

### 🛡️ Status Monitoring & Fault Tolerance

* **HUD Floating Window**: Switch to a mini status window with a three‑color breathing light (green/cyan/red) that visually indicates the current working status, along with real‑time token consumption statistics.
* **⏱️ Speed Test Mode**: When enabled, the translation log displays the underlying connection port type and **millisecond‑level latency** for each request, aiding in performance tuning and network troubleshooting.
* **Human‑Friendly Error Mapping**: Converts common HTTP status codes (401, 429, 500, etc.) and network timeouts into clear Chinese action suggestions, helping users quickly pinpoint issues. (Note: Error messages are displayed in Chinese in the GUI; the mapping is designed for a Chinese‑speaking audience.)
* **Enforced Timeout Protection**: A built‑in timeout mechanism of 10–40 seconds prevents game logic from freezing due to slow API responses.

---

## 🚀 Quick Start

### Standard Mode (Line‑by‑Line Processing)

1. Launch the program and fill in your API Key and model information.
2. Click **Test Configuration** to verify connectivity.
3. After a successful test, click **Start Service**.
4. Manually edit the `AutoTranslator/Config.ini` in your game directory:
   ```ini
   [Service]
   Endpoint=CustomTranslate
   
   [Custom]
   Url=http://localhost:6800   # Port must match the one set in the GUI
   ```

---

### 📦 Batch Mode (Multi‑line Concurrency, Recommended for Text‑Heavy Games)

1. Check **📦 Batch Mode (Batching)** in the main interface.
2. Make sure you have correctly selected a glossary (`.txt`) path; the program will use this path to automatically locate the game’s `Config.ini`.
3. Click **Start Service** (the program will automatically modify and take over the game configuration).
4. Launch the game. The batch mode will be applied automatically during translation, and the current configuration file will be **backed up automatically**.

---

## 📂 Code Structure

```text
src/
├── main.cpp                     # Application entry point, UI mode switching and transition animations
├── MainWindow.cpp/h             # Classic mode main window and business logic
├── ModernWindow.cpp/h           # Modern mode main window
├── TranslationServer.cpp/h      # HTTP server, API interaction, and retry logic
├── XuaConfigHijacker.h          # Automatic game config modification and restoration component
├── GlossaryManager.h            # Glossary read/write and RAG injection logic
├── RegexManager.h               # Text pre- and post-processing regular expressions
├── HudWindow.cpp/h              # HUD floating status window
├── ModernUI.h                   # Modern mode component library (built-in editor, rendering proxy)
├── ConfigManager.cpp/h          # Config file (config.ini) serialization and management
└── TokenManager.cpp/h           # Token statistics and management
```

---

## 🛠️ Build Guide

### Requirements
- C++17 compatible compiler (MSVC 2019+, MinGW 8.1+, Clang 11+)
- Qt 6.2.0 or higher (including Qt Network, Qt Widgets modules)
- CMake 3.16 or higher

### Build Steps
```bash
git clone https://github.com/your-repo/XUnity-LLM-Translator-GUI.git
cd XUnity-LLM-Translator-GUI
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=C:/Qt/6.5.0/msvc2019_64   # Replace with your Qt path
cmake --build . --config Release
```

### Notes
- If using MinGW, make sure `CMAKE_PREFIX_PATH` points to the correct Qt installation directory.
- The compiled executable will be located in the `build/Release/` folder.

---

> **For Secondary Developers**:
> To ensure perfect alignment and the best visual experience for UI elements, if you plan to further develop the interface, please try to keep the main window’s pixel size rigidly around `500 x 832`. (The window now supports free scaling, but this size remains the design reference.)

## 📦 Deployment & Packaging

### Dependency Collection
Use Qt’s `windeployqt` tool to collect runtime dependencies:
```bash
windeployqt --release --compiler-runtime XUnity-LLM-Translator-GUI.exe
```

### Single‑File Packaging
To create a single executable, you can use **Enigma Virtual Box** or **BoxedApp Packer** to bundle the dependencies into the main program. Remember to preserve the necessary Qt plugin directories (e.g., `platforms`, `styles`).

---

## 📝 License

This project is open‑source under the **MIT** license. You are free to use, modify, and distribute it, provided that the original copyright notice is retained.

---

> 📖 中文版: [README.md](README.md)
