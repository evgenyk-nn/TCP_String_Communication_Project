// client.c (работает норм со словами)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#ifdef _WIN32
#include <winsock2.h>
#include <stdint.h>
typedef intptr_t ssize_t;
typedef int socklen_t;
#else
#include <arpa/inet.h>
#include <unistd.h>
#endif
#include <sys/types.h>

#define PORT 8081
#define BUFFER_SIZE 64

// Функция для отправки данных на сервер
void send_data(int client_socket, const char *data)
{
    ssize_t send_result = send(client_socket, data, strlen(data), 0);
    if (send_result == -1)
    {
        perror("Error sending data");
    }
}

// Функция для получения данных от сервера
void receive_data(int client_socket, char *buffer)
{
    ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received == -1)
    {
        perror("Error receiving data");
    }
    else if (bytes_received == 0)
    {
        // Сервер закрыл соединение
        printf("Server closed the connection\n");
    }
    else
    {
        buffer[bytes_received] = '\0'; // Добавляем завершающий нуль для корректного вывода
        printf("Server response: %s\n", buffer);
    }
}

int main()
{
    int client_socket;
    struct sockaddr_in server_addr;

    // Создание сокета
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Настройка адреса сервера
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Подключение к серверу
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Error connecting to server");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    char input_buffer[BUFFER_SIZE];

    // Ввод данных с консоли и отправка на сервер
    while (1)
    {
        printf("Enter a word to send to the server (or type 'exit' to quit): ");
        fgets(input_buffer, BUFFER_SIZE, stdin);

        // Удаление символа новой строки из введенных данных
        size_t len = strlen(input_buffer);
        if (len > 0 && input_buffer[len - 1] == '\n')
        {
            input_buffer[len - 1] = '\0';
        }

        // Проверка на выход
        if (strcmp(input_buffer, "exit") == 0)
        {
            break;
        }

        // Отправка данных на сервер
        send_data(client_socket, input_buffer);

        // Получение и вывод ответа от сервера
        char response_buffer[BUFFER_SIZE];
        receive_data(client_socket, response_buffer);
    }

    // Закрытие сокета
    close(client_socket);

    return 0;
}