cmake_minimum_required(VERSION 3.16)

# Project Name and Language
# 项目名称与语言 (C++)
project(XUnityTranslatorCPP LANGUAGES CXX)

# Generate compile_commands.json (Useful for IDEs like VSCode/Clangd for intellisense)
# 生成 compile_commands.json (对于 VSCode/Clangd 等 IDE 的代码补全非常有用)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==============================================================================
# ⚠️ CRITICAL CONFIGURATION / 关键配置 ⚠️
#
# Qt Installation Path:
# You must modify this path to match your local Qt installation.
# Ensure you use forward slashes '/', not backslashes '\'.
#
# Qt 安装路径：
# 您必须修改此路径以匹配您本地的 Qt 安装位置。
# 请确保使用正斜杠 '/'，不要使用反斜杠 '\'。
# ==============================================================================
set(CMAKE_PREFIX_PATH "C:/Qt/6.10.1/mingw_64")

# ==============================================================================
# C++ Standard & Qt Build Tools
# C++ 标准与 Qt 构建工具
# ==============================================================================

# Require C++ 17
# 强制要求 C++ 17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt Meta-Object Compiler (MOC) automation
# Automatically runs moc for files containing Q_OBJECT
# 自动处理元对象编译器 (MOC)，用于处理包含 Q_OBJECT 的文件
set(CMAKE_AUTOMOC ON)

# Qt Resource Compiler (RCC) automation (.qrc files)
# 自动处理资源编译器 (RCC)
set(CMAKE_AUTORCC ON)

# Qt UI Compiler (UIC) automation (.ui files)
# 自动处理 UI 编译器 (UIC)
set(CMAKE_AUTOUIC ON)

# ==============================================================================
# Dependencies / 依赖项
# ==============================================================================

# Find required Qt6 modules: Widgets (GUI), Network (HTTP), Core (Base)
# 查找必要的 Qt6 模块：Widgets (界面), Network (网络), Core (核心)
find_package(Qt6 REQUIRED COMPONENTS Widgets Network Core)

# ==============================================================================
# Target Definition / 目标定义
# ==============================================================================

# Define the executable and list all source files
# 定义可执行文件并列出所有源文件
add_executable(XUnityTranslatorCPP
    src/main.cpp
    src/ConfigManager.h src/ConfigManager.cpp
    src/TranslationServer.h src/TranslationServer.cpp
    src/MainWindow.h src/MainWindow.cpp
    src/httplib.h 
    src/json.hpp
    src/moil.ico
    src/GlossaryManager.h
    src/RegexManager.h
    logo.rc
)

# Add 'src' directory to include headers
# Essential for verifying #include "json.hpp" works correctly
# 将 'src' 目录添加到头文件搜索路径
# 这对于确保 #include "json.hpp" 能被正确找到至关重要
target_include_directories(XUnityTranslatorCPP PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Link against Qt libraries
# 链接 Qt 库
target_link_libraries(XUnityTranslatorCPP PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Core
)

# ==============================================================================
# Platform Specific Settings / 平台特定设置
# ==============================================================================

# Windows: Console Window Settings
# Windows 下控制台窗口设置
if(WIN32)
    # Uncomment the following line to HIDE the console window (Production Mode)
    # Keep commented to SEE logs in the console (Debug Mode)
    # 取消下方注释以“隐藏”控制台黑框 (发布模式)
    # 保持注释状态以便在黑框中查看日志 (调试模式)
    
    set_target_properties(XUnityTranslatorCPP PROPERTIES WIN32_EXECUTABLE ON)
endif()