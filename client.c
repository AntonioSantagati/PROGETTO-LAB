#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include<arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define N_THREADS 30



void* job(void*arg){
    //variabili utili 
    int* n=(int*)arg;
    int socket_descriptor;
    struct sockaddr_in server_addr;
    char server_message[2000],client_message[2000];

    //clean buffers
    memset(server_message,'\0',sizeof(server_message));
    memset(client_message,'\0',sizeof(client_message));

    //1. create socket
    socket_descriptor = socket(AF_INET,SOCK_STREAM,0);

    //2. assign SOCK_TIPE, PORT, IP DEL SERVER
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(3490);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
   

    //3. send connectio reuquest to server 
    connect(socket_descriptor, (struct sockaddr *)&server_addr, sizeof(server_addr));

    
    //4. send message 
    sprintf(client_message, "Connection established with Client %d\n",n);
    send(socket_descriptor, client_message,strlen(client_message),0);

    //5 receive
    recv(socket_descriptor,server_message,sizeof(server_message),0);
    printf("server message: %s\n",server_message);

    //6. close socket
    close(socket_descriptor);
    sleep(5);
    pthread_exit(NULL);
    
}


int main(int argc, char *argv[]){
    
    pthread_t t[N_THREADS];
    for (int i = 0; i < N_THREADS; i++)
    {   
        pthread_create(&t[i], NULL, &job, (void*)i);      
    }
    

    for ( int i = 0; i < N_THREADS; i++)
    {
        pthread_join(t[i], NULL);
    }

    return 0;
    
}