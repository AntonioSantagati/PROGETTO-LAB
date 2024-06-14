#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vlc/vlc.h>
#include <time.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 6969
#define BUFFER_SIZE 4096

long long current_time_millis() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts); // CLOCK_MONOTONIC è una scelta comune, può essere anche CLOCK_REALTIME
    return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

int init_socket() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    char buffer[BUFFER_SIZE] = {0};
    char authentication[BUFFER_SIZE];
    char username[BUFFER_SIZE] = {0};
    char password[BUFFER_SIZE] = {0};
    char video_choice[BUFFER_SIZE];
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    server_address.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Connect failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
    read(sock, buffer, BUFFER_SIZE);
    printf("%s", buffer);
    fgets(username, BUFFER_SIZE, stdin);
    username[strcspn(username, "\n")] = '\0';  // Remove newline character
    send(sock, username, strlen(username), 0);

    // // Get password
    memset(buffer, 0, BUFFER_SIZE);
    read(sock, buffer, BUFFER_SIZE);
    printf("%s", buffer);
    fgets(password, BUFFER_SIZE, stdin);
    password[strcspn(password, "\n")] = '\0';  // Remove newline character
    send(sock, password, strlen(password), 0);

    // // Authentication response
    memset(buffer, 0, BUFFER_SIZE);
    read(sock, buffer, BUFFER_SIZE);
    strcpy(authentication, buffer);
    printf("%s", buffer);
    //send(sock,authentication,strlen(authentication),0);
    memset(buffer, 0, BUFFER_SIZE);
    if(strcmp (authentication, "Authenticated\n")==0){
            read(sock, buffer, BUFFER_SIZE);
            if(strcmp (buffer,"User already logged\n")==0){
                printf("%s", buffer);
                return -1;
            }
                printf("Lista dei video disponibili:\n");
                printf("%s", buffer);
            

        // User selects a video
            printf("Choose a video to be displayed...\n");
            fgets(video_choice, BUFFER_SIZE, stdin);
            video_choice[strcspn(video_choice, "\n")] = 0; // Remove newline character
            send(sock, video_choice, strlen(video_choice), 0);
    
    //if(strcmp (authentication, "Authenticated\n")==0){
            return sock;
    }

    else{
        return -1;
    }
}

static int open_media(void *opaque, void **datap, uint64_t *sizep) {
    int sock = *(int *)opaque;
    *datap = opaque;
    *sizep = 0; // Unknown size
    return 0; // Success
}

static ssize_t read_media(void *opaque, unsigned char *buf, size_t len) {
    int sock = *(int *)opaque;
    ssize_t bytes_read = read(sock, buf, len);
    if (bytes_read < 0) {
        perror("Read failed");
        return 0; // Return 0 to signal EOF to VLC
    }
    return bytes_read;
}

static int seek_media(void *opaque, uint64_t offset) {
    // Implement seek if needed
    return -1; // Seek not supported
}

static void close_media(void *opaque) {
    // No additional cleanup needed
}

int main() {
    long long start_time = current_time_millis();
    int sock = init_socket();
    if (sock < 0){
        return 1;
    }

    const char *vlc_args[] = {
            "--no-xlib"
        };

    libvlc_instance_t *vlc_instance = libvlc_new(0, vlc_args);
    if (!vlc_instance) {
        fprintf(stderr, "libvlc initialization failed\n");
        close(sock);
        return EXIT_FAILURE;
    }

    libvlc_media_player_t *mp = libvlc_media_player_new(vlc_instance);
    if (!mp) {
        fprintf(stderr, "libvlc media player creation failed\n");
        libvlc_release(vlc_instance);
        close(sock);
        return EXIT_FAILURE;
    }

    libvlc_media_t *m = libvlc_media_new_callbacks(
        vlc_instance, open_media, read_media, seek_media, close_media, &sock
    );
    if (!m) {
        fprintf(stderr, "libvlc media creation failed\n");
        libvlc_media_player_release(mp);
        libvlc_release(vlc_instance);
        close(sock);
        return EXIT_FAILURE;
    }

    libvlc_media_player_set_media(mp, m);
    libvlc_media_release(m);

    libvlc_media_player_play(mp);
    long long end_time = current_time_millis();
    long long elapsed_time = end_time - start_time;
    printf("%2lld",elapsed_time);
    printf("Streaming video...\nPress send to pause/play, 'q' to quit.\n");

    char command;
    while ((command = getchar())) {
        switch (command) {
            case '\n':
                libvlc_media_player_pause(mp);
                break;
            case 'q':
                send(sock, &command, 1, 0);
                libvlc_media_player_stop(mp);
                libvlc_media_player_release(mp);
                libvlc_release(vlc_instance);
                close(sock);
                return 0;
            default:
                continue;
        }
    }

/*cleanup:
    libvlc_media_player_stop(mp);
    libvlc_media_player_release(mp);
    libvlc_release(vlc_instance);
    close(sock);
    return 0;*/
    
}
