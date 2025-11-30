#include "ConsoleUI.hpp"

#include <conio.h>

namespace ui
{
ConsoleUI* ConsoleUI::instance = nullptr;

ConsoleUI::ConsoleUI()
    : running(false), hConsole(INVALID_HANDLE_VALUE), historyIndex(0), cursorPosition(0)
{
    instance = this;

    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

ConsoleUI::~ConsoleUI()
{
    stop();
    SetConsoleCtrlHandler(consoleCtrlHandler, FALSE);
    if (instance == this) instance = nullptr;
}

BOOL WINAPI ConsoleUI::consoleCtrlHandler(DWORD ctrlType)
{
    switch (ctrlType)
    {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
        {
            if (instance)
            {
                if (instance->shutdownCallback) instance->shutdownCallback();
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            return TRUE;
        }
        default:
            return FALSE;
    }
}

void ConsoleUI::setShutdownCallback(std::function<void()> callback) { shutdownCallback = callback; }

void ConsoleUI::clearInputLine()
{
    if (hConsole == INVALID_HANDLE_VALUE) return;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) return;

    savedCursorPosition = csbi.dwCursorPosition;

    COORD coord;
    coord.X = 0;
    coord.Y = csbi.dwCursorPosition.Y;
    SetConsoleCursorPosition(hConsole, coord);

    DWORD written;
    FillConsoleOutputCharacter(hConsole, ' ', csbi.dwSize.X, coord, &written);

    SetConsoleCursorPosition(hConsole, coord);
}

void ConsoleUI::restoreInputLine() { std::cout << "> " << currentInput << std::flush; }

void ConsoleUI::moveCursorToInputStart()
{
    if (hConsole == INVALID_HANDLE_VALUE) return;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hConsole, &csbi))
    {
        COORD coord;
        coord.X = 0;
        coord.Y = csbi.dwCursorPosition.Y;
        SetConsoleCursorPosition(hConsole, coord);
    }
}

void ConsoleUI::setCursorPosition(size_t pos)
{
    if (hConsole == INVALID_HANDLE_VALUE) return;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hConsole, &csbi))
    {
        COORD coord;
        coord.X = static_cast<SHORT>(2 + pos);
        coord.Y = csbi.dwCursorPosition.Y;
        SetConsoleCursorPosition(hConsole, coord);
    }
}

void ConsoleUI::redrawInputLine(const std::string& input)
{
    if (hConsole == INVALID_HANDLE_VALUE) return;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) return;

    COORD coord;
    coord.X = 0;
    coord.Y = csbi.dwCursorPosition.Y;
    SetConsoleCursorPosition(hConsole, coord);

    DWORD written;
    FillConsoleOutputCharacter(hConsole, ' ', csbi.dwSize.X, coord, &written);
    SetConsoleCursorPosition(hConsole, coord);

    std::cout << "> " << input << std::flush;

    setCursorPosition(cursorPosition);
}

void ConsoleUI::addToHistory(const std::string& command)
{
    if (command.empty()) return;

    if (!commandHistory.empty() && commandHistory.back() == command) return;

    commandHistory.push_back(command);

    if (commandHistory.size() > MAX_HISTORY_SIZE) commandHistory.erase(commandHistory.begin());

    historyIndex = commandHistory.size();
}

void ConsoleUI::navigateHistoryUp(std::string& input)
{
    if (commandHistory.empty()) return;

    if (historyIndex > 0)
    {
        historyIndex--;
        input = commandHistory[historyIndex];
        currentInput = input;
        cursorPosition = input.length();
        redrawInputLine(input);
    }
}

void ConsoleUI::navigateHistoryDown(std::string& input)
{
    if (commandHistory.empty()) return;

    if (historyIndex < commandHistory.size())
    {
        historyIndex++;

        if (historyIndex >= commandHistory.size())
        {
            input.clear();
            currentInput.clear();
            cursorPosition = 0;
        }
        else
        {
            input = commandHistory[historyIndex];
            currentInput = input;
            cursorPosition = input.length();
        }

        redrawInputLine(input);
    }
}

void ConsoleUI::handleKeyPress(char ch, std::string& input)
{
    if (ch == '\b' || ch == 127)  // backspace
    {
        if (!input.empty() && cursorPosition > 0)
        {
            input.erase(cursorPosition - 1, 1);
            cursorPosition--;
            currentInput = input;

            std::lock_guard<std::mutex> lock(consoleMutex);
            redrawInputLine(input);
        }
    }
    else if (ch >= 32 && ch <= 126)  // ASCII
    {
        input.insert(cursorPosition, 1, ch);
        cursorPosition++;
        currentInput = input;

        std::lock_guard<std::mutex> lock(consoleMutex);
        redrawInputLine(input);
    }
}

void ConsoleUI::handleExtendedKey(int extendedCode, std::string& input)
{
    switch (extendedCode)
    {
        case 72:  // arrow up
        {
            std::lock_guard<std::mutex> lock(consoleMutex);
            navigateHistoryUp(input);
            break;
        }
        case 80:  // arrow down
        {
            std::lock_guard<std::mutex> lock(consoleMutex);
            navigateHistoryDown(input);
            break;
        }
        case 75:  // arrow left
        {
            std::lock_guard<std::mutex> lock(consoleMutex);
            if (cursorPosition > 0)
            {
                cursorPosition--;
                setCursorPosition(cursorPosition);
            }
            break;
        }
        case 77:  // arrow right
        {
            std::lock_guard<std::mutex> lock(consoleMutex);
            if (cursorPosition < input.length())
            {
                cursorPosition++;
                setCursorPosition(cursorPosition);
            }
            break;
        }
        case 71:  // home
        {
            std::lock_guard<std::mutex> lock(consoleMutex);
            cursorPosition = 0;
            setCursorPosition(cursorPosition);
            break;
        }
        case 79:  // end
        {
            std::lock_guard<std::mutex> lock(consoleMutex);
            cursorPosition = input.length();
            setCursorPosition(cursorPosition);
            break;
        }
        case 83:  // delete
        {
            std::lock_guard<std::mutex> lock(consoleMutex);
            if (cursorPosition < input.length())
            {
                input.erase(cursorPosition, 1);
                currentInput = input;
                redrawInputLine(input);
            }
            break;
        }
        default:
            break;
    }
}

void ConsoleUI::startInputLoop(InputHandler handler)
{
    running.store(true, std::memory_order_release);

    inputThread = std::thread(
        [this, handler]()
        {
            std::string input;
            cursorPosition = 0;
            historyIndex = commandHistory.size();

            {
                std::lock_guard<std::mutex> lock(consoleMutex);
                std::cout << "> " << std::flush;
            }

            while (running.load(std::memory_order_acquire))
            {
                if (_kbhit())
                {
                    int ch = _getch();

                    if (ch == '\r' || ch == '\n')
                    {
                        {
                            std::lock_guard<std::mutex> lock(consoleMutex);
                            std::cout << std::endl;
                        }

                        if (!input.empty())
                        {
                            std::string inputCopy = input;

                            addToHistory(inputCopy);

                            input.clear();
                            currentInput.clear();
                            cursorPosition = 0;
                            historyIndex = commandHistory.size();

                            if (handler) handler(inputCopy);
                        }

                        if (running.load(std::memory_order_acquire))
                        {
                            std::lock_guard<std::mutex> lock(consoleMutex);
                            std::cout << "> " << std::flush;
                            cursorPosition = 0;
                        }
                    }
                    else if (ch == 0 || ch == 0xE0 || ch == 224)
                    {
                        if (_kbhit())
                        {
                            int extendedCode = _getch();
                            handleExtendedKey(extendedCode, input);
                        }
                    }
                    else
                    {
                        handleKeyPress(static_cast<char>(ch), input);
                    }
                }
                else
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
        });
}

void ConsoleUI::printLog(const std::string& msg)
{
    std::lock_guard<std::mutex> lock(consoleMutex);

    clearInputLine();

    std::cout << msg;

    if (msg.empty() || msg.back() != '\n') std::cout << std::endl;

    if (running.load(std::memory_order_acquire)) restoreInputLine();
}

void ConsoleUI::stop()
{
    bool expected = true;
    if (!running.compare_exchange_strong(expected, false, std::memory_order_acq_rel)) return;

    if (inputThread.joinable())
    {
        std::thread::id currentThreadId = std::this_thread::get_id();
        std::thread::id inputThreadId = inputThread.get_id();

        if (currentThreadId == inputThreadId)
        {
            inputThread.detach();
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (inputThread.joinable()) inputThread.join();
        }
    }
}

void ConsoleUI::setCurrentInput(const std::string& s)
{
    std::lock_guard<std::mutex> lock(consoleMutex);
    currentInput = s;
}
}  // namespace ui