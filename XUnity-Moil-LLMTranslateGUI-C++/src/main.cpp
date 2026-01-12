#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    // 初始化应用程序对象，管理 GUI 程序的控制流和主要设置
    // Initialize the application object, managing the GUI application's control flow and main settings
    QApplication app(argc, argv);

    // 设置应用程序 UI 风格为 "Fusion" 
    // (Fusion 是 Qt 提供的一种跨平台风格，通常比默认风格更现代且一致)
    // Set application UI style to "Fusion"
    // (Fusion is a cross-platform style provided by Qt, usually more modern and consistent than the default style)
    app.setStyle("Fusion"); 

    // 创建主窗口实例
    // Create the main window instance
    MainWindow w;

    // 显示主窗口 (默认是隐藏的，必须调用 show)
    // Show the main window (hidden by default, must call show)
    w.show();

    // 进入应用程序的主事件循环，开始处理事件（如点击、重绘等）
    // Enter the application's main event loop, start processing events (clicks, repaints, etc.)
    return app.exec();
}