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

**XUnity LLM Translator GUI** is a local translation relay tool designed for Unity games. It acts as a lightweight local HTTP forwarding server, bridging requests from **XUnity.AutoTranslator** to various large language models (such as Grok, DeepSeek, OpenAI, Gemini, Ollama, etc.).

The project is built with **C++17 / Qt**, aiming to provide **low latency** and **high concurrency** while offering a stable, intuitive visual configuration interface.

---

## 🖼️ UI Previews

### Classic Mode (Old vs New)

| Old Version | Current Version |
| :--: | :--: |
| <img src="docs/ui_classic_old.png"> | <img src="docs/ui_classic_new.png"> |

<p align="center">
<em>Fixed pixel layout · Sliding glossary window · New feature buttons</em>
</p>

---

### Modern Mode

<p align="center">
<img src="docs/ui_modern.png">
</p>

<p align="center">
<em>Glassmorphism UI · Dynamic gradient borders · Sliding glossary panel · Real‑time opacity adjustment</em>
</p>


---

## 🔄 Comparison with Early C++ Version

| Feature | Early C++ Version | Current Version |
| :--- | :---: | :---: |
| UI Architecture | Single main window | **Dual‑mode UI (Classic / Modern)** |
| Visual Style | Fixed theme | **Classic + Modern (Glassmorphism)** |
| Built‑in Glossary Editor | ❌ No | **✅ Built‑in sliding glossary editor** |
| Batch Translation | ❌ Line‑by‑line only | **✅ Batch mode with concurrent packing** |
| Config Auto‑management | ❌ Manual configuration | **✅ Partial automation with automatic backup** |
| Context Pollution Protection | Basic placeholder protection | **✅ Anti‑Bleed multi‑line isolation** |
| Glossary System | RAG auto‑supplement | **RAG + visual editing** |
| UI Animation System | Basic Qt | **Modern animations + glassmorphism rendering** |
| HUD Status Window | ✅ Supported | ✅ Supported + Token statistics |
| API Key Round‑Robin | ✅ | ✅ |
| Hot Reload | ✅ | ✅ |
| Concurrency Thread Pool | Not precisely defined | **64–256** |
| Error Messages | Basic mapping | **More comprehensive HTTP error hints** |

---

## 🛠️ Core Features

### 🎨 Dual‑Mode Interface

- **Classic Mode**: Lightweight and stable, ideal for older hardware.
- **Modern Mode**: Glassmorphism design with dynamic gradient borders and real‑time opacity adjustment for a modern look.
- **Built‑in Glossary Editor**: Sliding panel with syntax highlighting for source and target terms, making glossary maintenance easy.

---

### 🧠 Translation Logic

- **Batch Mode (Multi‑line Packing)**
  Automatically takes over `Config.ini` to enable concurrent batch translation, significantly boosting throughput for text‑intensive games. The original configuration is backed up automatically when the tool is launched.

- **Anti‑Bleed (Context Pollution Prevention)**
  Protects special markers like `[LF]` and `<T_0>` before sending requests, and instructs the model to treat each line as an independent fragment, preventing hallucinated connections between lines.

- **Glossary Self‑Evolution (RAG)**
  Automatically reads the glossary file as context during translation and intelligently extracts new terms from model responses to add them to the glossary, enabling continuous improvement.

---

### 🚀 Concurrency & Service Control

- **Asynchronous Thread Pool**: Built with `httplib` to support 64–256 concurrent requests, handling high‑load scenarios.
- **API Key Round‑Robin**: Supports multiple comma‑separated API keys; the system distributes requests automatically to avoid rate limits on a single key.
- **Dynamic Hot Reload**: Changes to model name, API key, system prompt, or sampling temperature take effect on the next request without restarting the service.

---

### 🛡️ Status Monitoring & Fault Tolerance

- **HUD Floating Window**: Switchable mini status window with a three‑color status light (green/cyan/red) indicating current state, plus real‑time token consumption tracking.
- **User‑Friendly Error Mapping**: Converts common HTTP status codes (401, 429, 500, etc.) and network timeouts into clear Chinese (or English) actionable suggestions.
- **Forced Timeout Protection**: Built‑in 10–40 second timeout mechanism prevents game logic from hanging due to slow API responses.

---

## 🚀 Quick Start

### Standard Mode (Line‑by‑Line)

1. Launch the program, fill in your API key and model information.
2. Click **Test Configuration** to verify connectivity.
3. After success, click **Start Service**.
4. Manually edit `AutoTranslator/Config.ini` in the game directory:
   ```ini
   [Service]
   Endpoint=CustomTranslate
   
   [Custom]
   Url=http://localhost:6800   # Must match the port set in the GUI
   ```

---

### 📦 Batch Mode (Multi‑line Concurrent – Recommended for Text‑Intensive Games)

1. Check **📦 Batch Mode (Batching)** in the main window.
2. Ensure the glossary (`.txt`) path is correctly set – the program will use it to locate the game's `Config.ini`.
3. Click **Start Service** (the program will automatically modify and take over the game configuration).
4. Launch the game. The batch mode will be applied during translation, and **the original configuration file will be automatically backed up**.

---

## 📂 Code Structure

```text
src/
├── main.cpp                     # Application entry, UI mode switching & transitions
├── MainWindow.cpp/h             # Classic mode main window and business logic
├── ModernWindow.cpp/h           # Modern mode main window
├── TranslationServer.cpp/h      # HTTP server, API interaction & retry logic
├── XuaConfigHijacker.h          # Game config auto‑modification and backup component
├── GlossaryManager.h            # Glossary file read/write & RAG injection logic
├── RegexManager.h               # Pre‑/post‑processing regex handling
├── HudWindow.cpp/h              # HUD floating status window
├── ModernUI.h                   # Modern mode UI component library (built‑in editor, rendering proxy)
├── ConfigManager.cpp/h          # Config file (config.ini) serialization read/write
└── TokenManager.cpp/h           # Token statistics management
```

---

## 🛠️ Build Guide

### Requirements
- C++17 compiler (MSVC 2019+, MinGW 8.1+, Clang 11+)
- Qt 6.2.0 or higher (including Qt Network, Qt Widgets, etc.)
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
- If using MinGW, ensure `CMAKE_PREFIX_PATH` points to the correct Qt installation.
- The compiled executable will be located in `build/Release/`.

---

> **For developers**:
> To ensure perfect alignment of UI elements and the best visual experience, if you plan to customize the interface, please hard‑limit the main window size to approximately `500 x 832` pixels.

## 📦 Deployment & Packaging

### Dependency Collection
Use Qt's `windeployqt` tool to collect runtime dependencies:
```bash
windeployqt --release --compiler-runtime XUnity-LLM-Translator-GUI.exe
```

### Single‑File Packaging
To generate a single‑file executable, you can use tools like **Enigma Virtual Box** or **BoxedApp Packer** to bundle dependencies into the main program. Remember to include necessary Qt plugin directories (e.g., `platforms`, `styles`).

---

## 📝 License

This project is open‑sourced under the **MIT** license. You are free to use, modify, and distribute it, but must retain the original copyright notice.

---

> 📖 中文版本: [README.md](README.md)
