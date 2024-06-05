#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <unistd.h>

#define N_THREADS 3
#define BUFFER_SIZE 4096
#define PORT 6969
#define IP_ADDRESS "127.0.0.1"

int connections = 0;
int counter = 0;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;




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


int authenticate(const char *username, const char *password, int client_descriptor){
    char *file_password;
    char *file_username;
    char *line;
    size_t len;
    char *read;

    FILE *check_user= fopen("check_user.txt","r");
    if (check_user==NULL){
        perror("Error to open file");
        return EXIT_FAILURE;
    }
    while((read = getline(&line, &len, check_user)) != -1){
        //line[strcspn(line, "\n")] = 0;  // Rimuovi il carattere di newline
        //printf("%s\n", line);
        //file_username = strtok(line, ",");
        //file_password = strtok(NULL, ",");
        if (file_username != NULL && file_password != NULL) {
            printf("%s,%s\n",file_username,file_password);
            printf("%s,%s\n",username,password);
            if (strcmp(file_username, username) == 0 && strcmp(file_password, password) == 0) {
                printf("Authenticated!\n");
                send(client_descriptor, "Authenticated\n", strlen("Authenticated\n"), 0);
                fclose(check_user);
                return 0;
            }
            return 1;
        }
        return 1;
    }
}

void *task(void *arg) {
    int client_descriptor = (int)(intptr_t)arg; 
    char *line;
    char buffer[BUFFER_SIZE];

    printf("request username...\n");
    send(client_descriptor,"Username: ",strlen("Username: "),0);
    read(client_descriptor,buffer,BUFFER_SIZE);
    char username[BUFFER_SIZE];
    strcpy(username,buffer);
    memset(buffer,0,BUFFER_SIZE);
    printf("Username recieved..\n");


    printf("request passowrd...\n");
    send(client_descriptor, "Password: ", strlen("Password: "), 0);
    read(client_descriptor, buffer, BUFFER_SIZE);
    char password[BUFFER_SIZE];
    strcpy(password, buffer);
    memset(buffer, 0, BUFFER_SIZE);
    printf("password recieved..\nAuthentication...\n");

    if(verify(username,password,client_descriptor)==0){

        FILE *video_file = fopen("sium.mp4", "rb");
            if (video_file == NULL) {
                perror("Error opening video file");
                close(client_descriptor);
                return NULL;
            }

        while (!feof(video_file)) {
            size_t bytes_read = fread(buffer, 1, sizeof(buffer), video_file);
            if (bytes_read > 0) {
                printf("Bytes letti dal file video: %zu\n", bytes_read);
                ssize_t bytes_sent = send(client_descriptor, buffer, bytes_read, 0);
                if (bytes_sent == -1) {
                    perror("Error sending data");
                    break;
                }
                printf("Bytes inviati al client: %zu\n", bytes_sent);
            }
        }
        fclose(video_file);

    }
    else{
        //printf("%d",authenticate(username, password,client_descriptor));
        //printf("%s-%s",username,password);
        send(client_descriptor, "Authentication Failed\n", strlen("Authentication Failed\n"), 0);
        
    }

    printf("Chiusura connessione client...\n");
    close(client_descriptor);
    

    pthread_mutex_lock(&m);
    counter++;
    connections--;
    pthread_mutex_unlock(&m);

    return NULL;
}

int main(int argc, char *argv[]) {
    int socket_descriptor, client_size, client_socket;
    struct sockaddr_in server_addr, client_address;

    socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_descriptor == -1) {
        perror("Error creating socket");
        return EXIT_FAILURE;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(IP_ADDRESS);

    if (bind(socket_descriptor, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding socket");
        close(socket_descriptor);
        return EXIT_FAILURE;
    }

    if (listen(socket_descriptor, N_THREADS) < 0) {
        perror("Error listening on socket");
        close(socket_descriptor);
        return EXIT_FAILURE;
    }

    pthread_t threads[N_THREADS];

    while (1) {
        client_size = sizeof(client_address);
        client_socket = accept(socket_descriptor, (struct sockaddr *)&client_address, (socklen_t *)&client_size);
        if (client_socket < 0) {
            perror("Error accepting client");
            continue;
        }
        pthread_mutex_lock(&m);
        printf("Client accepted...\n");
        pthread_create(&threads[connections], NULL, &task, (void *)(intptr_t)client_socket); 
        connections++;
        pthread_mutex_unlock(&m);
    }

    close(socket_descriptor);
    return 0;
}
