#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define N_THREADS 3
#define BUFFER_SIZE 4096
#define PORT 6969
#define IP_ADDRESS "127.0.0.1"
#define VIDEO "galaxy.mp4"

int connections = 0;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void *task(void *arg) {
    int client_descriptor = *(int *)arg;
    char buffer[BUFFER_SIZE];

    FILE *video_file = fopen(VIDEO, "rb");
    if (video_file == NULL) {
        perror("Error opening video file");
        close(client_descriptor);
        return NULL;
    }

    while (!feof(video_file)) {
        size_t bytes_read = fread(buffer, 1, sizeof(buffer), video_file);
        if (bytes_read > 0) {
            ssize_t bytes_sent = send(client_descriptor, buffer, bytes_read, 0);
            if (bytes_sent == -1) {
                perror("Error sending data");
                break;
            }
        }
    }

    fclose(video_file);
    close(client_descriptor);

    pthread_mutex_lock(&m);
    connections--;
    pthread_mutex_unlock(&m);

    return NULL;
}

int main() {
    int socket_descriptor, client_size, client_socket;
    struct sockaddr_in server_addr, client_address;
    pthread_t threads[N_THREADS];

    // Creazione del socket
    socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_descriptor == -1) {
        perror("Error creating socket");
        return EXIT_FAILURE;
    }

    // Configurazione dell'indirizzo del server
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(IP_ADDRESS);

    // Binding del socket
    if (bind(socket_descriptor, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding socket");
        close(socket_descriptor);
        return EXIT_FAILURE;
    }

    // Listening del socket
    if (listen(socket_descriptor, N_THREADS) < 0) {
        perror("Error listening on socket");
        close(socket_descriptor);
        return EXIT_FAILURE;
    }

    while (1) {
        client_size = sizeof(client_address);
        client_socket = accept(socket_descriptor, (struct sockaddr *)&client_address, (socklen_t *)&client_size);
        if (client_socket < 0) {
            perror("Error accepting client");
            continue;
        }

        pthread_mutex_lock(&m);
        if (connections < N_THREADS) {
            printf("Client accepted...\n");
            pthread_create(&threads[connections], NULL, &task, &client_socket);
            connections++;
        } else {
            printf("Maximum connections reached\n");
            close(client_socket);
        }
        pthread_mutex_unlock(&m);
    }

    close(socket_descriptor);

    return 0;
}
