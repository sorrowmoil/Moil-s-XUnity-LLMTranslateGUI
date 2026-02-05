# XUnity LLM Translator GUI

<div align="center">

<h2>
  <a href="README_US.md">English</a> | <a href="README.md">中文</a>
</h2>

</div>

<div align="center">

<img src="https://img.shields.io/badge/license-MIT-green" height="25">  
<img src="https://img.shields.io/badge/Qt-6.x-blue" height="25">  
<img src="https://img.shields.io/badge/C++-17-orange" height="25">
<img src="https://img.shields.io/badge/Platform-Windows-lightgrey" height="25">

</div>

---

## 🇬🇧 English Version

### Introduction
**XUnity LLM Translator GUI** is a high-performance translation proxy designed for Unity games. It functions as a local HTTP relay server, efficiently bridging **XUnity.AutoTranslator** requests to various Large Language Models (LLMs) such as Grok, DeepSeek, OpenAI, Gemini, and more.

The project has fully migrated to a **C++ / Qt** architecture, providing ultra-low latency, high concurrency stability, and a modern interactive experience.

---

### ✨ C++ Version Key Features

#### 🚀 Exceptional Performance
- **Native Async Architecture**: Built with C++17 and the Qt event loop for zero-blocking request dispatching.
- **Concurrent Thread Management**: Integrated high-performance thread pool (via `httplib`) to handle multiple requests simultaneously, significantly increasing translation throughput.
- **API Key Polling**: Supports multiple comma-separated API Keys with automatic round-robin load balancing.

#### 🧠 Intelligent Logic
- **Hot Reload**: Modify Model Name, API Key, System Prompt, or Temperature on the fly without stopping the service. Changes take effect immediately.
- **Self-Evolving Glossary (RAG)**: Bridges with XUnity's glossary files. The model references existing terms and **actively discovers/extracts** new terminology from original texts via prompt engineering.
- **Local Escape Freezing**: Built-in text preprocessing to protect game code escape characters, HTML tags, and special placeholders (e.g., `ZMCZ`), preventing them from being mistranslated by the LLM.

#### 🎨 Modern Interaction
- **HUD Mini-Window Mode**: One-click switch to a minimalist HUD overlay. Features a **3-color status light** (Green: Idle, Blue: Working, Red: Error) and real-time Token counter.
- **Human-Friendly Error System**: Maps obscure HTTP error codes (401, 429, 500) and network timeouts (999) into clear, actionable advice in English/Chinese.
- **10-Second Timeout Detection**: Enforces a strict timeout mechanism for API responses to prevent game logic deadlocks caused by provider server hangs.
- **Multi-language & Themes**: Full bilingual support with one-click switching between Modern Dark and Fusion Light themes.
- **Smart Presets**: Built-in endpoint presets for major API providers (OpenAI, DeepSeek, xAI, SiliconFlow) and local models (Ollama, LM Studio).

---

### ⚠️ Version Comparison

| Feature | C++ Enhanced (Moil) | Python Original (Fox) |
| :--- | :---: | :---: |
| Latency | **Ultra-low (Native)** | Moderate (Interpreter) |
| Hot Reload | ✅ Full Support | ❌ Restart Required |
| Glossary | ✅ Auto-Extraction | ❌ Read-only |
| Error Handling | ✅ Friendly Mapping | ⚠️ Basic Errors |
| HUD Status Light | ✅ Built-in | ❌ None |
| UI Stability | ✅ High (Qt Native) | ⚠️ Layout Shifts |

---

### 🚀 Quick Start

1. **Get the App**: Download the latest `.exe` from [Releases](../../releases).
2. **Configure Endpoint**: Select a preset provider or manually enter an API address (OpenAI compatible).
3. **Connectivity Test**: Click **Test Config**. The system validates all keys and outputs a detailed summary in the logs.
4. **Start Service**: Click **Start Service**.
5. **Setup Plugin**: Edit `AutoTranslator/Config.ini` in your game folder:
     ```ini
   [Service]
   Endpoint=CustomTranslate
   FallbackEndpoint=
   ...
   [Custom]
   Url=http://localhost:6800
   EnableShortDelay=true
   DisableSpamChecks=true
   
   ```

---

### 📂 File Structure (C++ Core)

```text
src/
├── TranslationServer.cpp/h     # Core HTTP server & relay logic
├── MainWindow.cpp/h           # GUI logic & error mapping
├── HudWindow.cpp/h            # HUD overlay & breathing light
├── LoadingOverlay.h           # Async button animation component
├── GlossaryManager.h          # Term extraction & RAG injection
├── ConfigManager.cpp/h        # Structured config persistence
├── TokenManager.cpp/h         # Token statistics & tracking
└── main.cpp                   # Entry point & style initialization
```

---

### 🛠️ Compilation & Development

#### C++ Version
- **Requirements**: C++17 compatible compiler (MSVC 2019+, MinGW 8.1+), Qt 6.2.0+, CMake 3.16+.
- **Build Steps**:
  ```bash
  mkdir build && cd build
  cmake .. -DCMAKE_PREFIX_PATH=/your/qt/path
  cmake --build . --config Release
  ```

#### Python Version (Legacy)
- **Install Deps**: `pip install ttkbootstrap openai requests`
- **Run**: `python XUnity-Moli@LLMTtranslatedGUI.py`

---

### 📦 Deployment & Packaging

- **C++ Version**: Use `windeployqt` to collect runtime dependencies. Can be wrapped into a single executable using **Enigma Virtual Box**.
- **Python Version**: Recommended to package using `PyInstaller --onefile --windowed`.

---

### 🎯 Development Roadmap

- [x] **High-performance Engine**: Native C++ refactor completed.
- [x] **Config Hot Reload**: Real-time parameter adjustment implemented.
- [x] **Smart Error System**: 10s timeout & HTTP status localization.
- [x] **Self-Evolving Glossary**: Automatic term learning during translation.
- [x] **HUD Mini-Mode**: Status monitoring with 3-color light.
- [ ] **Multi-Endpoint Load Balancing**: Automatic request distribution.

---

### 📝 License
This project is licensed under the **MIT** License.

---

> 📖 **中文版本请参阅**: [README.md](README.md)
