#include <exception>

#include "ChatApplication.hpp"

int main()
{
    std::cout << "[INFO] Application started" << std::endl;

    app::ChatApplication app;
    app.init();
    app.run();

    return 0;
}
