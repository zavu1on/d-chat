#include <iostream>
#include <locale>

int main()
{
    setlocale(LC_ALL, "ru_RU.UTF-8");
    std::cout << "hello world" << std::endl;
    std::cout << "привет мир!" << std::endl;

    return 0;
}
