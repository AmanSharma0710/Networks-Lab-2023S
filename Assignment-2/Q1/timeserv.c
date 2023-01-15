#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <time.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
  
#define MAXLINE 1024 
  
int main() { 
    int sockfd, n; 
    socklen_t len;
    char buffer[MAXLINE]; 
    struct sockaddr_in servaddr, cliaddr; 
      
    // Create socket file descriptor 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if ( sockfd < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
      
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr)); 
      
    servaddr.sin_family    = AF_INET; 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(8181); 
      
    // Bind the socket with the server address 
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,  
            sizeof(servaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    
    printf("\nServer Running....\n");
    while(1){
        len = sizeof(cliaddr);
        n = recvfrom(sockfd, (char *)buffer, MAXLINE, 0, 
                ( struct sockaddr *) &cliaddr, &len); 
        memset(buffer, 0, sizeof(buffer));
        time_t t = time(NULL);
		struct tm tm = *localtime(&t);
		sprintf(buffer, "Current server date and time is: %d-%d-%d %d:%d:%d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
        sendto(sockfd, (const char *)buffer, strlen(buffer), 0, 
                (const struct sockaddr *) &cliaddr, len); 
    }
    close(sockfd);
    return 0; 
} 