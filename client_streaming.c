#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vlc/vlc.h>

#define BUFFER_SIZE 4096
#define PORT 6969
#define IP_ADDRESS "127.0.0.1"
#define VIDEO "received_video.mp4"

int running = 1;

ssize_t receive_bytes(int socket_descriptor, FILE *video_file) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received = recv(socket_descriptor, buffer, BUFFER_SIZE, 0);
    if (bytes_received > 0) {
        fwrite(buffer, 1, bytes_received, video_file);
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
    FILE *video_file;
    libvlc_instance_t *vlc_instance;
    libvlc_media_player_t *media_player;
    libvlc_media_t *media;
    libvlc_event_manager_t *event_manager;

    char buffer[BUFFER_SIZE];

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

    // Connessione al server
    if (connect(socket_descriptor, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting to server");
        close(socket_descriptor);
        return EXIT_FAILURE;
    }

    // Ricezione dei dati video dal server
    video_file = fopen(VIDEO, "wb");
    if (video_file == NULL) {
        perror("Error opening video file");
        close(socket_descriptor);
        return EXIT_FAILURE;
    }

    ssize_t bytes_received;
    while ((bytes_received = receive_bytes(socket_descriptor, video_file)) > 0) {
        printf("Received %zd bytes from server\n", bytes_received);
    }

    fclose(video_file);

    // Inizializzazione di VLC per la riproduzione del video
    vlc_instance = libvlc_new(0, NULL);
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
        usleep(100000); // Aspetta un po' per gestire gli eventi
    }

    printf("Stopping video...\n");

    libvlc_media_player_stop(media_player);
    libvlc_media_player_release(media_player);
    libvlc_release(vlc_instance);

    close(socket_descriptor);

    return 0;
}
