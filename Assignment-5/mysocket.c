#include "mysocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h> 
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ctype.h>
#include <pthread.h>
#include <errno.h>

#define QUEUE_SIZE 10
#define MAX_INPUT_SIZE 5000
#define MAX_PACKET_SIZE 1000

char **Send_Message = NULL;
char **Received_Message = NULL;
pthread_t R, S;

void *R_thread(void *arg){
    //Receive messages from the network and put them in the Received_Message

}

void *S_thread(void *arg){
    //Send messages from the Send_Message to the network

}



int my_socket(int domain, int type, int protocol){
    //We only support sockets of type SOCK_MyTCP
    if(type != SOCK_MyTCP){
        errno = ESOCKTNOSUPPORT;
        return -1;
    }

    //If a socket of this type is already open, return error
    if(Send_Message!=NULL){
        perror("Socket already open");
        return -1;
    }

    //Create a socket
    int sockfd = socket(domain, SOCK_STREAM, protocol);
    
    //If there are no errors spawn two threads R and S and declare data structures to store
    if(sockfd != -1){
        //Declare data structures for book keeping
        Send_Message = (char**)malloc(QUEUE_SIZE*sizeof(char*));
        Received_Message = (char**)malloc(QUEUE_SIZE*sizeof(char*));
        for(int i=0; i<QUEUE_SIZE; i++){
            Send_Message[i] = NULL;
            Received_Message[i] = NULL;
        }
        //Spawn threads R and S
        pthread_create(&R, NULL, &R_thread, NULL);
        pthread_create(&S, NULL, &S_thread, NULL);
    }

    return sockfd;
}

int my_bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen){
    return bind(sockfd, addr, addrlen);
}

int my_connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen){
    return connect(sockfd, addr, addrlen);
}

int my_accept(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict addrlen){
    return accept(sockfd, addr, addrlen);
}

int my_listen(int sockfd, int backlog){
    return listen(sockfd, backlog);
}

int my_close(int sockfd){
    //Sleep for 5 seconds
    sleep(5);

    //Handle the case when the socket is not open
    if(Send_Message==NULL){
        perror("Socket not open");
        return -1;
    }

    //Kill threads R and S here after dealing with the race condition
    pthread_cancel(R);
    pthread_cancel(S);

    //Free the data structures
    for(int i=0; i<QUEUE_SIZE; i++){
        if(Send_Message[i]!=NULL){
            free(Send_Message[i]);
        }
        if(Received_Message[i]!=NULL){
            free(Received_Message[i]);
        }
    }
    free(Send_Message);
    free(Received_Message);
    Send_Message = NULL;
    Received_Message = NULL;

    return close(sockfd);
}

int my_send(int sockfd, const void *buf, size_t len, int flags);

int my_recv(int sockfd, void *buf, size_t len, int flags);