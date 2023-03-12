#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<poll.h> 
#include<string.h>
#include<sys/types.h>
#include<time.h>
#include<sys/socket.h>
#include <arpa/inet.h>
#include<netinet/in.h>
#include<ctype.h>

int my_socket(int domain, int type, int protocol);

int my_bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen);

int my_connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen);

int my_accept(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict addrlen);

int my_listen(int sockfd, int backlog);

int my_close(int sockfd);
