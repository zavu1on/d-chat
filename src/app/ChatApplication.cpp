#include "ChatApplication.hpp"

ChatApplication::~ChatApplication() { shutdown(); }

void ChatApplication::init()
{
    setlocale(LC_ALL, "ru_RU.UTF-8");
    std::cout << "=== Echo сервер ===" << std::endl;

    chatService = std::make_shared<ChatService>();

    std::cout << "Введите порт сервера: ";
    int serverPort;
    std::cin >> serverPort;

    server = std::make_unique<TCPServer>(serverPort, chatService);

    server->start();
    std::cout << "Сервер запущен и слушает входящие соединения..." << std::endl;

    std::cout << "Введите порт клиента (порт назначения): ";
    int clientPort;
    std::cin >> clientPort;

    client = std::make_unique<TCPClient>("127.0.0.1", clientPort, chatService);

    running = true;
}

void ChatApplication::run()
{
    if (!running)
    {
        std::cerr << "Ошибка: приложение не инициализировано!" << std::endl;
        return;
    }

    std::cout << "=== Чат готов к работе! ===" << std::endl;

    while (running)
    {
        std::string message;
        std::cout << "\nВведите сообщение: ";
        std::getline(std::cin >> std::ws, message);

        if (message == "/exit")
        {
            std::cout << "Завершение работы..." << std::endl;
            running = false;
            break;
        }

        std::string response = client->sendMessage(message);

        std::cout << "Отправлено: " << message << std::endl;
        std::cout << "Получено: " << response << std::endl;
    }
}

void ChatApplication::shutdown()
{
    if (server) server->stop();

    if (serverThread.joinable()) serverThread.join();

    running = false;
}
