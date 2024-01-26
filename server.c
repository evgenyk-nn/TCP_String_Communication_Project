// server.c (работает норм со словами)
#include <ctype.h>
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
#define MAX_CLIENTS 5
#define BUFFER_SIZE 64

// Объявление мьютекса
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Структура данных для передачи в поток
struct ThreadData
{
    int client_socket;
    char response_buffer[BUFFER_SIZE]; // Добавляем буфер для ответа
    int client_id;                     // Добавляем поле для client_id
};

// Объявление статической переменной для идентификации клиентов
static int client_id_counter = 0;
// Новая переменная для отслеживания текущего количества клиентов
static int current_clients = 0;

// Объявление функции process_request
void process_request(char *request, char *response);

// Функция для обработки запроса в отдельном потоке
void *handle_client(void *data)
{
    struct ThreadData *thread_data = (struct ThreadData *)data;

    printf("Client %d connected\n", thread_data->client_id);

    while (1)
    {
        ssize_t bytes_received = recv(thread_data->client_socket, thread_data->response_buffer, BUFFER_SIZE, 0);
        if (bytes_received == -1)
        {
            perror("Error receiving data");
            break;
        }
        else if (bytes_received == 0)
        {
            // Клиент закрыл соединение
            break;
        }
        else
        {
            {
                pthread_mutex_lock(&mutex);

                process_request(thread_data->response_buffer, thread_data->response_buffer);

                pthread_mutex_unlock(&mutex);
            }

            ssize_t send_result = send(thread_data->client_socket, thread_data->response_buffer, BUFFER_SIZE, 0);
            if (send_result == -1)
            {
                perror("Error sending data");
                break;
            }
            else if (send_result == 0)
            {
                printf("Client %d disconnected due to connection failure\n", thread_data->client_id);
                break;
            }
        }
    }

    // Уменьшаем счетчик активных клиентов после отключения клиента
    pthread_mutex_lock(&mutex);
    current_clients--;
    pthread_mutex_unlock(&mutex);

    // Закрытие соединения с клиентом
    close(thread_data->client_socket);

    // Завершение потока
    printf("Client %d disconnected\n", thread_data->client_id);
    free(thread_data);
    pthread_exit(NULL);
}

// Функция для обработки запроса
void process_request(char *request, char *response)
{
    for (int i = 0; i < strlen(request); ++i)
    {
        response[i] = toupper(request[i]);
    }
}

int main()
{
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Error binding socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    listen(server_socket, 5);
    printf("Server listening on port %d\n", PORT);

    while (1)
    {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket == -1)
        {
            perror("Error accepting connection");
            continue;
        }

        pthread_mutex_lock(&mutex);

        if (current_clients >= MAX_CLIENTS)
        {
            printf("Maximum number of clients reached. Rejecting new connection.\n");
            close(client_socket);
        }
        else
        {
            printf("Client %d connected\n", client_id_counter);

            current_clients++;

            struct ThreadData *thread_data = malloc(sizeof(struct ThreadData));
            if (thread_data == NULL)
            {
                perror("Error allocating memory for thread data");
                close(client_socket);
            }
            else
            {
                thread_data->client_socket = client_socket;
                memset(thread_data->response_buffer, 0, sizeof(thread_data->response_buffer));
                thread_data->client_id = client_id_counter++;

                pthread_t thread;
                if (pthread_create(&thread, NULL, handle_client, thread_data) != 0)
                {
                    perror("Error creating thread");
                    free(thread_data);
                    close(client_socket);
                }
                else
                {
                    pthread_detach(thread);
                }
            }
        }

        pthread_mutex_unlock(&mutex);
    }

    close(server_socket);

    return 0;
}