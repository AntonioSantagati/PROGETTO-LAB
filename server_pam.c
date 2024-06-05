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

static struct pam_conv conv = {
    misc_conv,
    NULL
};


int authenticate(const char *username, const char *password) {
    pam_handle_t *pamh = NULL;
    int retval;
    struct pam_conv pam_conversation = {
        misc_conv,
        NULL
    };

    retval = pam_start("check_user", username, &pam_conversation, &pamh);
    if (retval != PAM_SUCCESS) {
        fprintf(stderr, "Error initializing PAM: %s\n", pam_strerror(pamh, retval));
        return 1;
    }

    retval = pam_set_item(pamh, PAM_AUTHTOK, "cane");
    if (retval != PAM_SUCCESS) {
        fprintf(stderr, "Error setting PAM password: %s\n", pam_strerror(pamh, retval));
        pam_end(pamh, retval);
        return 1;
    }

    retval = pam_authenticate(pamh, 0);
    if (retval != PAM_SUCCESS) {
        fprintf(stderr, "Authentication failed: %s\n", pam_strerror(pamh, retval));
        pam_end(pamh, retval);
        return 1;
    }

    retval = pam_acct_mgmt(pamh, 0);
    if (retval != PAM_SUCCESS) {
        fprintf(stderr, "Account management failed: %s\n", pam_strerror(pamh, retval));
        pam_end(pamh, retval);
        return 1;
    }

    pam_end(pamh, PAM_SUCCESS);
    return 0;
}

    
    


void *task(void *arg) {
    int client_descriptor = (int)(intptr_t)arg; // Cast a intptr_t per evitare warning
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


    //authentication PAM
    if (authenticate(username,password) == 0 ){
        send(client_descriptor, "Authenticated\n", strlen("Authenticated\n"), 0);

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

        printf("Chiusura connessione client...\n");
        fclose(video_file);
        close(client_descriptor);
    }
    else{
        printf("%d",authenticate(username, password));
        printf("%s-%s",username,password);
        send(client_descriptor, "Authentication Failed\n", strlen("Authentication Failed\n"), 0);
        close(client_descriptor);

    }

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
        pthread_create(&threads[connections], NULL, &task, (void *)(intptr_t)client_socket); // Cast a intptr_t per evitare warning
        connections++;
        pthread_mutex_unlock(&m);
    }

    close(socket_descriptor);
    return 0;
}
