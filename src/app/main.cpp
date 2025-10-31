#include "ChatApplication.hpp"

// todo
// src/shared/db/DatabaseConnection.cpp - сервис подключения к бд, аппаратная реализация
// src/domain/messages/IMessageRepository.hpp - интерфейс репозитория сообщений
// src/infra/messages/MessageRepository.cpp - реализация репозитория сообщений
// + бизнес сущности и dto для MessageRepository

// src/service/crypto/Cryptography.cpp - сервис шифрования, аппаратная реализация
// src/domain/crypto/EncryptionService.cpp - бизнес логика шифрования сообщений
// + бизнес сущности и dto для EncryptionService

// src/shared/network/SocketServer.cpp - сервис сервера на сокетах, "аппаратная" реализация
// src/domain/chat/ChatService.cpp - бизнес логика приложения, работа с бд (MessageRepository),
// crypto (EncryptionService) и тд
// + бизнес сущности и dto для ChatService
// src/domain/chat/IChatServer.hpp - абстрактный интерфейс сервера чата
// src/infra/chat/ChatServer.cpp - реализация сервера чата, доступ к бизнес логике только через
// ChatService

// src/shared/network/SocketClient.cpp - сервис tcp клиента, "аппаратная" реализация
// src/domain/chat/IChatClient.hpp - абстрактный интерфейс клиента чата (без бд и crypto)
// src/infra/chat/ChatClient.hpp - абстрактный интерфейс клиента чата (без бд и crypto)

int main()
{
    ChatApplication app;

    // todo add crypto
    // todo add db and all entities
    // todo complete CLI version
    // todo add ui

    app.init();
    app.run();

    return 0;
}