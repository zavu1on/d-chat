#pragma once
#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

class ConsoleUI
{
private:
    std::mutex consoleMutex;
    std::thread inputThread;
    std::atomic<bool> running;
    std::string currentInput;

public:
    using InputHandler = std::function<void(const std::string&)>;

    ConsoleUI();

    void startInputLoop(InputHandler handler);
    void printLog(const std::string& msg);
    void stop();
    void setCurrentInput(const std::string& s);
};
