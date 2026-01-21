# XUnity LLM Translator GUI

<div align="center">

<h2>
  <a href="README_US.md">English</a> | <a href="README.md">中文</a>
</h2>

</div>

<div align="center">

<img src="https://img.shields.io/badge/license-MIT-green" height="40">  
<img src="https://img.shields.io/badge/python-3.9+-blue" height="40">  
<img src="https://img.shields.io/badge/C++-17-orange" height="40">

</div>

---

## 🇬🇧 English Version

### Introduction
**XUnity LLM Translator GUI** is a high-performance local HTTP server that bridges **XUnity.AutoTranslator** (Unity game translation plugin) with Large Language Models (LLMs) such as OpenAI, Gemini, Claude, DeepSeek, etc.

- **Source Code**: Python and C++ implementations are available in the `main` branch.  
- **Executables**: Precompiled `.exe` files are provided in [Releases](../../releases).  
- **Versions**:  
  - **Fox** → Original stable version (smooth animations, simple UI)  
  - **Moil** → Enhanced successor with multi-language UI, glossary, themes, and advanced features  

---

### 🔄 Version History

#### Python Versions
- **Fox**: Stable base implementation with smooth animations  
- **Moil Edition**: Enhanced features with some UI trade-offs  

#### C++ Version
- **Complete Refactor**: Ultra-low latency, high-concurrency modern implementation  

---

### ✨ Key Features

#### 🚀 Core (All Versions)
- High performance, designed for game-level concurrency  
- Multi-key polling with round-robin rotation  
- Modern UI with real-time log monitoring  
- Auto-save of last configuration  

#### 🎯 Moil Python Ultimate Edition
- 🌍 Bilingual UI (English/Chinese)  
- 🎨 Theme switching (darkly/flatly)  
- 📚 Intelligent glossary with automatic term extraction  
- ⚡ Config import/export  
- 🔧 Improved error handling & multi-key support  

#### ⚡ C++ Version
- Ultra-low latency native performance  
- Advanced thread pool for high concurrency  
- Regex-based pre/postprocessor loading  
- RAG integration for context injection  
- Smart sampling for speed/learning balance  

---

### ⚠️ Version Comparison

| Feature              | Moil Python | Fox Python | C++ |
|----------------------|-------------|------------|-----|
| Multi-language UI    | ✅          | ❌         | ✅  |
| Theme Switching      | ✅          | ❌         | ✅  |
| Glossary System      | ✅          | ❌         | ✅  |
| Animation Smoothness | ⚠️ Less     | ✅ Smooth  | ✅  |
| UI Layout            | ⚠️ Issues   | ✅ Stable  | ✅  |
| Performance          | Good        | Good       | Excellent |
| Setup Complexity     | Easy        | Easy       | Requires compilation |

---

### 🚀 Quick Start

#### Option 1: Run Executable (Recommended)
1. Download the latest `.exe` from [Releases](../../releases)  
2. Double-click to run  
3. Configure API address, key, and port, then click **Start Server**

#### Option 2: Run from Source (Developers)
```bash
pip install ttkbootstrap openai requests
python XUnity-Moil的LLMTranslateGUI.py
```

---

### 🧩 File Structure

#### C++ Version (`src/`)
```text
src/
├── ConfigManager.cpp / .h         # Config loading/saving
├── GlossaryManager.h              # Glossary term management
├── htestlib.h                     # Utility header
├── json.hpp                       # JSON parser (nlohmann/json)
├── main.cpp                       # Entry point
├── MainWindow.cpp / .h           # GUI logic
├── RegexManager.cpp / .h         # Regex-based pre/postprocessor
├── translate.ico                  # Application icon
├── TranslationServer.h           # HTTP server interface
├── HudWindow.cpp/.h              # HUD light
```

#### Python Version (`Python/`)
```text
Python/
├── XUnity-Moli@LLMTtranslatedGUI.py   # Main GUI script (Moil version)
├── moli.ico                           # Application icon
```

---

### 🛠️ Usage

#### Configure XUnity.AutoTranslator
Edit `AutoTranslator/Config.ini`:
```ini
[Service]
Endpoint=http://localhost:6800
```

#### Glossary (Moil & C++)
- Enable **Self-Evolution**  
- Select `_Substitutions.txt`  
- Tool respects existing terms and learns new ones  

---

### ⚙️ Compilation (Developers)

#### C++ Version
- Requirements: CMake 3.16+, Qt 6.x, C++17 compiler (MSVC/MinGW)  
- Build:
  ```bash
  mkdir build && cd build
  cmake ..
  cmake --build . --config Release
  ```
  > 💡 `--config Release` is required for multi-config generators like Visual Studio to build the optimized release version. On Linux/macOS, this flag is usually not needed.

#### Python Version
- **Moil**: Recommended (feature-rich)  
- **Fox**: Alternative (smooth animations, stable layout)  
- Dependencies via pip, no compilation needed  

---

### 🎯 Development Roadmap & TODO

<details>
<summary><strong>🚀 Performance Optimization</strong></summary>

- [x] ~~Optimize translation speed~~ - **Completed in Moil**: Multi-threaded HTTP server, context caching, API key rotation
- [x] ~~Add concurrency support~~ - **Completed in Moil**: ThreadingHTTPServer + thread pool support
</details>

<details>
<summary><strong>🌐 Language Support</strong></summary>

- [x] ~~Add English support~~ - **Completed in Moil**: Complete bilingual English/Chinese interface, real-time language switching
</details>

<details>
<summary><strong>🛠️ Code Refactoring</strong></summary>

- [x] ~~Refactor using other languages~~ - **Completed**: C++ version refactored, providing higher performance
- [ ] Further code optimization and modularization
</details>

<details>
<summary><strong>✨ New Features Added in Moil Version</strong></summary>

- [x] **Multi-language Interface**: Complete bilingual English/Chinese support
- [x] **Theme System**: Light/Dark theme switching
- [x] **Intelligent Glossary**: Self-evolving terminology management system
- [x] **Enhanced Configuration Management**: Config import/export functionality
- [x] **Tooltip System**: Detailed control function descriptions
- [x] **Smooth Animations**: Fade in/out effects
- [x] **Context Menu**: Right-click functionality in log area
- [x] **Multi-API Key Support**: Automatic key rotation
</details>

<details>
<summary><strong>🔮 Future Enhancements</strong></summary>

- [ ] Additional language interface support
- [ ] Advanced glossary management interface
-</details>

---

### 📝 License
MIT License. Free to fork and modify.

### 💡 Packaging
- Python: **PyInstaller**  
- C++: **Enigma Virtual Box** / **windeployqt**

---

> 📖 **中文版本请参阅**: [README.md](README.md)
