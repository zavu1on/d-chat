#include <iostream>

#include "SocketClient.hpp"
#include "SocketServer.hpp"

int main()
{
    // заблокировать конструктор копирования, оператор присваивания

    setlocale(LC_ALL, "ru_RU.UTF-8");
    std::cout << "Echo сервер" << std::endl;
    std::cout << "Введите порт: ";

    int port;
    std::cin >> port;

    SocketServer server(port);
    server.start(
        [](const char* buffer, int bufferSize, std::function<void(const std::string&)> sendCallback)
        {
            sendCallback(std::string(buffer, bufferSize));
            std::cout << "Получено новое сообщение: " << buffer << std::endl;
        });
    std::cout << "Сервер запущен в фоне" << std::endl;

    std::cout << "Чат готов к работе!" << std::endl;
    std::cout << "Введите порт клиента: ";
    std::cin >> port;

    std::cout << "Объект клиента успешно создан" << std::endl;

    while (true)
    {
        std::string message;

        std::cout << "Введите сообщение: ";
        std::cin >> message;

        SocketClient client("127.0.0.1", port);

        client.sendMessage(message);
        std::cout << "Отправлено сообщение: " << message << std::endl;

        std::cout << "Получено сообщение: " << client.receiveMessage() << std::endl;
    }

    return 0;
}