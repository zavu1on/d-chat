#pragma once

// clang-format off
#include <winsock2.h>
#include <windows.h>
// clang-format on

#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace ui
{
class ConsoleUI
{
private:
    std::mutex mutex;
    std::thread inputThread;
    std::atomic<bool> running;
    std::string currentInput;
    std::function<void()> shutdownCallback;

    HANDLE hConsole;
    COORD savedCursorPosition;

    std::vector<std::string> commandHistory;
    size_t historyIndex;
    static constexpr size_t MAX_HISTORY_SIZE = 100;

    size_t cursorPosition;

    void clearInputLine();
    void restoreInputLine();
    void redrawInputLine(const std::string& input);
    void moveCursorToInputStart();
    void setCursorPosition(size_t pos);

    void handleKeyPress(char ch, std::string& input);
    void handleExtendedKey(int extendedCode, std::string& input);

    void addToHistory(const std::string& command);
    void navigateHistoryUp(std::string& input);
    void navigateHistoryDown(std::string& input);

public:
    using InputHandler = std::function<void(const std::string&)>;

    static ConsoleUI* instance;  // todo remove Singleton
    static BOOL WINAPI consoleCtrlHandler(DWORD ctrlType);

    ConsoleUI();
    ~ConsoleUI();

    void startInputLoop(InputHandler handler);
    void printLog(const std::string& msg);
    void stop();
    void setCurrentInput(const std::string& s);
    void setShutdownCallback(std::function<void()> callback);
};
}  // namespace ui