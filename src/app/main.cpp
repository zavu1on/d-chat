#include <exception>

#include "ChatApplication.hpp"

int main()
{
    try
    {
        ChatApplication app;
        app.init();
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        return 1;
    }
    std::cout << "[INFO] Application finished" << std::endl;

    return 0;
}