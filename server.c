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

typedef struct {
    const char *username;
    const char *password;
} pam_auth_data;

static int pam_conversation(int num_msg, const struct pam_message **msg,
                            struct pam_response **resp, void *appdata_ptr) {
    pam_auth_data *data = (pam_auth_data *)appdata_ptr;
    struct pam_response *responses = calloc(num_msg, sizeof(struct pam_response));

    for (int i = 0; i < num_msg; ++i) {
        responses[i].resp = NULL;
        responses[i].resp_retcode = 0;

        switch (msg[i]->msg_style) {
            case PAM_PROMPT_ECHO_OFF:
                responses[i].resp = strdup(data->password);
                break;
            case PAM_PROMPT_ECHO_ON:
                responses[i].resp = strdup(data->username);
                break;
            case PAM_ERROR_MSG:
            case PAM_TEXT_INFO:
                break;
            default:
                free(responses);
                return PAM_CONV_ERR;
        }
    }

    *resp = responses;
    return PAM_SUCCESS;
}

int authenticate(const char *username, const char *password) {
    pam_handle_t *pamh = NULL;
    int retval;
    pam_auth_data data = {username, password};
    struct pam_conv pam_conversation1 = {pam_conversation, &data};

    retval = pam_start("check_user", username, &pam_conversation1, &pamh);
    if (retval != PAM_SUCCESS) {
        return 1;
    }

    retval = pam_authenticate(pamh, 0);
    if (retval != PAM_SUCCESS) {
        pam_end(pamh, retval);
        return 2;
    }

    retval = pam_acct_mgmt(pamh, 0);
    if (retval != PAM_SUCCESS) {
        pam_end(pamh, retval);
        return 3;
    }

    pam_end(pamh, PAM_SUCCESS);
    return 0;
}

void *task(void *arg) {
    int client_descriptor = (int)(intptr_t)arg;
    char buffer[BUFFER_SIZE];

    printf("request username...\n");
    send(client_descriptor, "Username: ", strlen("Username: "), 0);
    read(client_descriptor, buffer, BUFFER_SIZE);
    char username[BUFFER_SIZE];
    strcpy(username, buffer);
    memset(buffer, 0, BUFFER_SIZE);
    printf("Username received..\n");

    printf("request password...\n");
    send(client_descriptor, "Password: ", strlen("Password: "), 0);
    read(client_descriptor, buffer, BUFFER_SIZE);
    char password[BUFFER_SIZE];
    strcpy(password, buffer);
    memset(buffer, 0, BUFFER_SIZE);
    printf("Password received..\nAuthentication...\n");

    if (authenticate(username, password) != 0) {
        printf("%d",authenticate(username, password));
        printf("%zu-%zu",username,password);
        send(client_descriptor, "Authentication Failed\n", strlen("Authentication Failed\n"), 0);
        close(client_descriptor);
    } else {
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
                printf("Bytes read from video file: %zu\n", bytes_read);
                ssize_t bytes_sent = send(client_descriptor, buffer, bytes_read, 0);
                if (bytes_sent == -1) {
                    perror("Error sending data");
                    break;
                }
                printf("Bytes sent to client: %zu\n", bytes_sent);
            }
        }

        printf("Closing client connection...\n");
        fclose(video_file);
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
        pthread_create(&threads[connections], NULL, &task, (void *)(intptr_t)client_socket);
        connections++;
        pthread_mutex_unlock(&m);
    }

    close(socket_descriptor);
    return 0;
}
