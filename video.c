#include <stdio.h>
#include <stdlib.h>
#include <vlc/vlc.h>

// Callback per l'evento di fine riproduzione
void end_reached_callback(const libvlc_event_t *event, void *data) {
    int *running = (int *)data;
    *running = 0;  // Imposta a 0 per indicare che la riproduzione è terminata
}

int main() {
    // Nome del file video
    const char *file_name = "sium.mp4";

    // Inizializzazione libvlc
    libvlc_instance_t *inst;
    libvlc_media_player_t *mp;
    libvlc_media_t *m;
    libvlc_event_manager_t *event_manager;
    int running = 1;

    // Opzioni per la libreria VLC
    const char *vlc_args[] = {
        "--no-xlib",          // Disabilita l'uso di Xlib
         "x11"       // Forza l'uso dell'output video X11
    };

    // Creazione di una nuova istanza VLC
    inst = libvlc_new(2, vlc_args);
    if (inst == NULL) {
        fprintf(stderr, "Errore: impossibile creare l'istanza VLC.\n");
        return EXIT_FAILURE;
    }

    // Creazione di un nuovo media
    m = libvlc_media_new_path(inst, file_name);
    if (m == NULL) {
        fprintf(stderr, "Errore: impossibile aprire il file video.\n");
        libvlc_release(inst);
        return EXIT_FAILURE;
    }

    // Creazione del media player
    mp = libvlc_media_player_new_from_media(m);
    if (mp == NULL) {
        fprintf(stderr, "Errore: impossibile creare il media player.\n");
        libvlc_media_release(m);
        libvlc_release(inst);
        return EXIT_FAILURE;
    }

    // Rilascio del media (non è più necessario)
    libvlc_media_release(m);

    // Registrazione del callback per l'evento di fine riproduzione
    event_manager = libvlc_media_player_event_manager(mp);
    libvlc_event_attach(event_manager, libvlc_MediaPlayerEndReached, end_reached_callback, &running);

    // Avvio della riproduzione(forse bloccante)
    libvlc_media_player_play(mp);

    // Loop principale per tenere il programma in esecuzione finché la riproduzione non è terminata
    while (running) {
        // Attendi un breve periodo di tempo per evitare di consumare troppa CPU
        usleep(100000); // 100ms
    }

    // Arresto del media player
    libvlc_media_player_stop(mp);

    // Rilascio del media player
    libvlc_media_player_release(mp);

    // Rilascio dell'istanza VLC
    libvlc_release(inst);

    return EXIT_SUCCESS;
}
