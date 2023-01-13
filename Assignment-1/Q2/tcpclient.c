
/*    THE CLIENT PROCESS */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main()
{
	int			sockfd ;
	struct sockaddr_in	serv_addr;

	int i;
	char buf[100];
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(0);
	}
	serv_addr.sin_family	= AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port	= htons(20000);
	if ((connect(sockfd, (struct sockaddr *) &serv_addr,
						sizeof(serv_addr))) < 0) {
		perror("Unable to connect to server\n");
		exit(0);
	}

	printf("Enter arithmetic expression to be evaluated(-1 to end) and press enter:\n");
	while(1){
		//We will use getline to read input of unknown length
		size_t len = 10;	//Initial length of buffer
		char *line = (char *)malloc(len * sizeof(char));
		getline(&line, &len, stdin);
		//if we read -1(even with whitespace) we exit
		int idx = 0;
		for(int i = 0; i < strlen(line); i++){
			if(line[i] != ' ' && line[i] != '\t' && line[i]!='\n'){
				if(line[i]=='-' && idx==0){
					idx=1;
					continue;
				}
				if(line[i]=='1' && idx==1){
					idx=2;
					continue;
				}
				idx+=5;
			}
		}
		if(idx==2){
			//Send -1 so the server can exit
			send(sockfd, "-1\0", 3, 0);
			free(line);
			break;
		}
		//Send the expression to the server
		send(sockfd, line, strlen(line) + 1, 0);
		//Receive the result from the server
		for(i=0; i < 100; i++) buf[i] = '\0';
		recv(sockfd, buf, 100, 0);
		printf("The above expression evaluates to: %s\n", buf);
		free(line);
	}
	
	close(sockfd);
	return 0;

}

