#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define N_THREADS 10
#define BUFFER_SIZE 4096
#define PORT 6969
#define IP_ADDRESS "127.0.0.1"

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

typedef struct{
    int client_socket;
    char video_name[BUFFER_SIZE];
}client_info_t;

int verify(const char *username, const char *password, int client_descriptor){
    char file_username[BUFFER_SIZE];
    char file_password[BUFFER_SIZE];
    char line[BUFFER_SIZE];
    FILE *check_user = fopen("check_user.txt", "r");

    if (check_user == NULL){
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    while (fgets(line, sizeof(line), check_user)) {
        if (sscanf(line, "%s %s", file_username, file_password) == 2) {
            if (strcmp(file_username, username) == 0 && strcmp(file_password, password) == 0) {
                printf("Authenticated!\n");
                send(client_descriptor, "Authenticated\n", strlen("Authenticated\n"), 0);
                fclose(check_user);
                return 0;
            }
        }
    }

    fclose(check_user);
    return EXIT_FAILURE;
}

void *handle_client(void *arg) {
    client_info_t *client_info = (client_info_t *)arg;
    int client_socket = client_info->client_socket;
    char buffer[BUFFER_SIZE];
    char video[BUFFER_SIZE];

    // Invio della lista dei video al client
    char *video_names[] = {"valenzia", "sium", "genas", "galaxy"};
    char video_list[BUFFER_SIZE] = "";
    for (int i = 0; i < sizeof(video_names) / sizeof(char *); i++) {
        strcat(video_list, video_names[i]);
        strcat(video_list, "\n");
    }
    send(client_socket, video_list, strlen(video_list), 0);

    // Ricezione della scelta del video dal client
    memset(buffer, 0, BUFFER_SIZE);  // Pulizia del buffer prima della lettura
    if (read(client_socket, buffer, BUFFER_SIZE) <= 0) {
        perror("Error reading video choice from client");
        close(client_socket);
        free(client_info);
        return NULL;
    }

    snprintf(video, BUFFER_SIZE, "%s.mp4", buffer); // Formattazione sicura del nome del file
    printf("Selected video: %s\n", video);
    strcpy(client_info->video_name, video);

    // Apertura e invio del file video
    FILE *video_file = fopen(client_info->video_name, "rb");
    if (video_file == NULL) {
        perror("Error opening video file");
        close(client_socket);
        free(client_info);
        return NULL;
    }

    while (!feof(video_file)) {
        size_t bytes_read = fread(buffer, 1, sizeof(buffer), video_file);
        if (bytes_read > 0) {
            send(client_socket, buffer, bytes_read, 0);
        }
    }

    fclose(video_file);
    close(client_socket);
    free(client_info);
    return NULL;
}


int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating socket");
        return EXIT_FAILURE;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(IP_ADDRESS);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding socket");
        close(server_socket);
        return EXIT_FAILURE;
    }

    if (listen(server_socket, N_THREADS) < 0) {
        perror("Error listening on socket");
        close(server_socket);
        return EXIT_FAILURE;
    }

    printf("Server listening on %s:%d\n", IP_ADDRESS, PORT);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("Error accepting client");
            continue;
        }
        char buffer_verify[BUFFER_SIZE];

        printf("request username...\n");
        send(client_socket,"Username: ",strlen("Username: "),0);
        read(client_socket,buffer_verify,BUFFER_SIZE);
        char username[BUFFER_SIZE];
        strcpy(username,buffer_verify);
        memset(buffer_verify,0,BUFFER_SIZE);
        printf("Username recieved..\n");


        printf("request passowrd...\n");
        send(client_socket, "Password: ", strlen("Password: "), 0);
        read(client_socket, buffer_verify, BUFFER_SIZE);
        char password[BUFFER_SIZE];
        strcpy(password, buffer_verify);
        memset(buffer_verify, 0, BUFFER_SIZE);
        printf("password recieved..\nAuthentication...\n");

        if(verify(username,password,client_socket)==0){
            client_info_t *client_info = malloc(sizeof(client_info_t));
            client_info->client_socket = client_socket;
            pthread_t thread;
            pthread_create(&thread, NULL, handle_client, (void *)client_info);
            pthread_detach(thread);
        }
        else{
            send(client_socket, "Authentication Failed\n", strlen("Authentication Failed\n"), 0);
            printf("Authentication Failed\n");
            return -1;

        }
    }

    close(server_socket);
    return 0;
}