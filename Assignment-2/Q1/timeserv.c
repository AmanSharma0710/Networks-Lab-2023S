// A Simple Client Implementation
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <poll.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
  
int main() { 
    int sockfd; 
    struct sockaddr_in servaddr; 
  
    // Creating socket file descriptor 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if ( sockfd < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr)); 
      
    // Server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(8181); 
    inet_aton("127.0.0.1", &servaddr.sin_addr); 
    
    int n, response_received = 0;
    socklen_t len; 
    char *hello = "Send time and date pls ;)"; 
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer)); 
    struct pollfd poll_set[1];
    poll_set[0].fd = sockfd;
    poll_set[0].events = POLLIN;
    //We wait for 3 seconds for the result, else we try again
    //If even after 5 times we don't get a response we say timed out and exit with failure
    for(int i=0; i<5; i++){
        sendto(sockfd, (const char *)hello, strlen(hello), 0, 
                (const struct sockaddr *) &servaddr, sizeof(servaddr)); 
        int poll_result = poll(poll_set, 1, 3000);
        if(poll_result > 0){
            response_received = 1;
            break;
        }
    }
    if(!response_received){
        printf("Timeout Exceeded\n");
        exit(EXIT_FAILURE);
    }
    n = recvfrom(sockfd, (char *)buffer, 1024, 0, ( struct sockaddr *) &servaddr, &len);
    printf("%s\n", buffer);
    close(sockfd); 
    return 0; 
} 