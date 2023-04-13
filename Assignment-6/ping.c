#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<netdb.h>
#include<string.h>

int main(){
    char *buf;
    size_t len = 0;
    getline(&buf, &len, stdin);
    buf[strlen(buf) - 1] = '\0'; // remove the newline character
    printf("Host name: %s\n", buf);
    struct hostent *host = gethostbyname(buf);
    if(host == NULL){
        printf("Error in gethostbyname");
        exit(1);
    }
    printf("IP address: %s\n", inet_ntoa(*((struct in_addr *)(host->h_addr_list[0]))));
    return 0;
}