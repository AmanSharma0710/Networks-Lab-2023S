#ifndef MY_SOCKET_H
#define MY_SOCKET_H

// Define the socket type for MyTCP
#define SOCK_MyTCP 100

//Include the standard socket header
#include <sys/socket.h>

//Function definitions
int my_socket(int domain, int type, int protocol);

int my_bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen);

int my_connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen);

int my_accept(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict addrlen);

int my_listen(int sockfd, int backlog);

int my_close(int sockfd);

int my_send(int sockfd, const void *buf, size_t len, int flags);

int my_recv(int sockfd, void *buf, size_t len, int flags);

#endif