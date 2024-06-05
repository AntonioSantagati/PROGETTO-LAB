#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <vlc/vlc.h>

#define BUFFER_SIZE 4096
#define PORT 6969
#define IP_ADDRESS "127.0.0.1"
#define VIDEO "received_video.mp4"

int running = 1;

typedef struct {
    int socket_descriptor;
    FILE *video_file;
    unsigned char buffer[BUFFER_SIZE];
} stream_t;

ssize_t receive_bytes(stream_t *stream) {
    ssize_t bytes_received = recv(stream->socket_descriptor, stream->buffer, BUFFER_SIZE, 0);
    if (bytes_received > 0) {
        printf("Received %zd bytes from server\n", bytes_received);
    } else if (bytes_received == 0) {
        printf("Server closed connection\n");
    } else {
        perror("Error receiving bytes");
    }
    return bytes_received;
}
void end_reached_callback(const libvlc_event_t *event, void *data) {
    int *running = (int *)data;
    *running = 0;  
}

int main() {
    int socket_descriptor;
    struct sockaddr_in server_addr;
    libvlc_instance_t *vlc_instance;
    libvlc_media_player_t *media_player;
    libvlc_media_t *media;
    libvlc_event_manager_t *event_manager;
    stream_t stream;

    //PAM
   
    char buffer[BUFFER_SIZE] = {0};
    char username[BUFFER_SIZE] = {0};
    char password[BUFFER_SIZE] = {0};




    // Create socket
    socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_descriptor == -1) {
        perror("Error creating socket");
        return EXIT_FAILURE;
    }

    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(IP_ADDRESS);

    if (connect(socket_descriptor, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting to server");
        close(socket_descriptor);
        return EXIT_FAILURE;
    }


    read(socket_descriptor, buffer, BUFFER_SIZE);
    printf("%s", buffer);
    fgets(username, BUFFER_SIZE, stdin);
    username[strcspn(username, "\n")] = '\0';  // Remove newline character
    send(socket_descriptor, username, strlen(username), 0);

    // Get password
    memset(buffer, 0, BUFFER_SIZE);
    read(socket_descriptor, buffer, BUFFER_SIZE);
    printf("%s", buffer);
    fgets(password, BUFFER_SIZE, stdin);
    password[strcspn(password, "\n")] = '\0';  // Remove newline character
    send(socket_descriptor, password, strlen(password), 0);

    // Authentication response
    memset(buffer, 0, BUFFER_SIZE);
    read(socket_descriptor, buffer, BUFFER_SIZE);
    printf("%s", buffer);

    if(strcmp (buffer, "Authenticated\n")==0){
        stream.socket_descriptor = socket_descriptor;
        stream.video_file = fopen(VIDEO, "wb");
        if (stream.video_file == NULL) {
            perror("Error opening video file");
            close(socket_descriptor);
            return EXIT_FAILURE;
        }
        ssize_t bytes_received;
        while ((bytes_received = receive_bytes(&stream)) > 0) {
            printf("Writing %zd bytes to file\n", bytes_received);
            fwrite(stream.buffer, 1, bytes_received, stream.video_file);
        }

        printf("Finished receiving video. Closing file.\n");
        fclose(stream.video_file);
    
        const char *vlc_args[] = {
            "--no-xlib",
            "x11"
        };

        vlc_instance = libvlc_new(2, vlc_args);
        if (vlc_instance == NULL) {
            fprintf(stderr, "Error creating VLC instance\n");
            close(socket_descriptor);
            return EXIT_FAILURE;
        }

        media = libvlc_media_new_path(vlc_instance, VIDEO);
        if (media == NULL) {
            fprintf(stderr, "Error creating VLC media\n");
            libvlc_release(vlc_instance);
            close(socket_descriptor);
            return EXIT_FAILURE;
        }

        media_player = libvlc_media_player_new_from_media(media);
        if (media_player == NULL) {
            fprintf(stderr, "Error creating VLC media player\n");
            libvlc_media_release(media);
            libvlc_release(vlc_instance);
            close(socket_descriptor);
            return EXIT_FAILURE;
        }

        libvlc_media_release(media);

        event_manager = libvlc_media_player_event_manager(media_player);
        libvlc_event_attach(event_manager, libvlc_MediaPlayerEndReached, end_reached_callback, &running);

        libvlc_media_player_play(media_player);
        printf("Playing video...\n");

        while (running) {
            printf("stat sleep..\n");
            usleep(120000);
            
        }

        printf("Stopping video...\n");

        libvlc_media_player_stop(media_player);
        libvlc_media_player_release(media_player);
        libvlc_release(vlc_instance);
    }

    else{
        printf("ERROR...\n");
    }
    close(socket_descriptor);

    return 0;


}
