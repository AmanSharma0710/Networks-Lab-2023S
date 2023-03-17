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


//We use a data structure to store the messages to be sent and received
//We use a circular queue to store the messages
typedef struct {
    char **messages;
    int *lengths;
    int *socket;
    int left;
    int right;
} Message_queue;

//We define Send_Message and Received_Message as global variables
Message_queue *Send_Message, *Received_Message;
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
    
    //If there are no errors spawn two threads R and S and declare data structures to store messages
    if(sockfd != -1){
        //Declare and initialize data structures for book keeping
        Send_Message = (Message_queue*)malloc(sizeof(Message_queue));
        Received_Message = (Message_queue*)malloc(sizeof(Message_queue));
        Send_Message->messages = (char**)malloc(QUEUE_SIZE*sizeof(char*));
        Received_Message->messages = (char**)malloc(QUEUE_SIZE*sizeof(char*));
        Send_Message->lengths = (int*)malloc(QUEUE_SIZE*sizeof(int));
        Received_Message->lengths = (int*)malloc(QUEUE_SIZE*sizeof(int));
        Send_Message->socket = (int*)malloc(QUEUE_SIZE*sizeof(int));
        Received_Message->socket = (int*)malloc(QUEUE_SIZE*sizeof(int));
        Send_Message->left = 0;
        Send_Message->right = 0;
        Received_Message->left = 0;
        Received_Message->right = 0;
        for(int i=0; i<QUEUE_SIZE; i++){
            Send_Message->messages[i] = NULL;
            Send_Message->lengths[i] = 0;
            Send_Message->socket[i] = 0;
            Received_Message->messages[i] = NULL;
            Received_Message->lengths[i] = 0;
            Received_Message->socket[i] = 0;
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
        if(Send_Message->messages[i]!=NULL){
            free(Send_Message->messages[i]);
            Send_Message->messages[i] = NULL;
        }
        if(Received_Message->messages[i]!=NULL){
            free(Received_Message->messages[i]);
            Received_Message->messages[i] = NULL;
        }
    }
    free(Send_Message->messages);
    free(Send_Message->lengths);
    free(Send_Message->socket);
    free(Received_Message->messages);
    free(Received_Message->lengths);
    free(Received_Message->socket);
    free(Send_Message);
    free(Received_Message);
    Send_Message = NULL;
    Received_Message = NULL;
    return close(sockfd);
}

int my_send(int sockfd, const void *buf, size_t len, int flags){    //The flags are ignored
    //Store the socket to be sent to and the message to be sent in the Send_Message, along with the length of the message
    //If the queue is full, sleep for 1 second and try again
    while(Send_Message->left == (Send_Message->right+1)%QUEUE_SIZE){
        sleep(1);
    }
    Send_Message->messages[Send_Message->right] = (char*)malloc(len*sizeof(char));
    memcpy(Send_Message->messages[Send_Message->right], buf, len);
    Send_Message->lengths[Send_Message->right] = len;
    Send_Message->socket[Send_Message->right] = sockfd;
    Send_Message->right = (Send_Message->right+1)%QUEUE_SIZE;
    return len;
}

int my_recv(int sockfd, void *buf, size_t len, int flags){  //The flags are ignored
    //If the queue is empty, sleep for 1 second and try again
    while(Received_Message->left == Received_Message->right){
        sleep(1);
    }
    //Read the message from the Received_Message and copy it to the buffer
    if(Received_Message->lengths[Received_Message->left] > len){
        memcpy(buf, Received_Message->messages[Received_Message->left], len);
    }
    else{
        memcpy(buf, Received_Message->messages[Received_Message->left], Received_Message->lengths[Received_Message->left]);
    }
    

}