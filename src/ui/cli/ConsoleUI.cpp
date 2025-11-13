#include "ConsoleUI.hpp"

namespace ui {

ConsoleUI::ConsoleUI() : running(false) {}

void ConsoleUI::startInputLoop(InputHandler handler)
{
    running = true;
    inputThread = std::thread(
        [this, handler]()
        {
            std::string input;
            while (running)
            {
                {
                    std::lock_guard<std::mutex> lock(consoleMutex);
                    std::cout << "> " << std::flush;
                }

                if (!std::getline(std::cin, input)) break;

                if (!running) break;
                if (handler) handler(input);

                {
                    std::lock_guard<std::mutex> lock(consoleMutex);
                    currentInput.clear();
                }
            }
        });
}

void ConsoleUI::printLog(const std::string& msg)
{
    std::lock_guard<std::mutex> lock(consoleMutex);

    std::cout << "\r" << std::string(currentInput.size() + 2, ' ') << "\r";
    std::cout << msg << std::endl;
    std::cout << "> " << currentInput << std::flush;
}

void ConsoleUI::stop()
{
    running = false;
    if (inputThread.joinable()) inputThread.join();
}

void ConsoleUI::setCurrentInput(const std::string& s)
{
    std::lock_guard<std::mutex> lock(consoleMutex);
    currentInput = s;
}

} // namespace ui