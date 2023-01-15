
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
	char buf[50];

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

	for(i=0; i < 50; i++) buf[i] = '\0';
	recv(sockfd, buf, 50, 0);
	if(strcmp(buf, "LOGIN:") != 0){
		printf("Unrecognised server response\n");
		close(sockfd);
		exit(0);
	}
	printf("%s ", buf);
	getline(&buf, 50, stdin);
	int input_length = strlen(buf);
	if(input_length>26){	//25 + 1 for newline
		printf("Input too long. Please try again.\n");
		close(sockfd);
		exit(0);
	}
	for(int i=input_length-1; i<50; i++){	//Overwrite the newline with nulls
		buf[i] = '\0';
	}
	send(sockfd, buf, 50, 0);
	for(i=0; i < 50; i++) buf[i] = '\0';
	recv(sockfd, buf, 50, 0);
	if(strcmp(buf, "NOT-FOUND") == 0){
		printf("Invalid username\n");
		close(sockfd);
		exit(0);
	}
	else if(strcmp(buf, "FOUND") != 0){
		printf("Unrecognised server response\n");
		close(sockfd);
		exit(0);
	}
	while(1){
		printf("Enter a command to be executed on the server(pwd, dir, cd, exit): ");
		for(i=0; i < 50; i++) buf[i] = '\0';
		getline(&buf, 50, stdin);
		input_length = strlen(buf);
		buf[--input_length] = '\0';	//Remove the newline
		if(input_length>50){
			printf("Input too long. Please try again.\n");
			continue;
		}

		send(sockfd, buf, 50, 0);

		//If the user enters exit, close the socket and exit without waiting for a response from the server
		if(strcmp(buf, "exit") == 0){
			close(sockfd);
			exit(0);
		}

		recv(sockfd, buf, 50, 0);
		if(strcmp(buf, "$$$$")==0){
			printf("Invalid command\n");
			continue;
		}
		else if(strcmp(buf, "####")==0){
			printf("Error in running command\n");
			continue;
		}
		printf("%s", buf);
		int response_end = 0;
		for(int i=0; i<50; i++){
			if(buf[i] == '\0'){
				response_end = 1;
				break;
			}
		}
		//Receive response from server in packets of 50 bytes until the server sends \0
		if(response_end == 0){
			while(1){
				//Clear the buffer and receive the next packet
				for(i=0; i < 50; i++) buf[i] = '\0';
				recv(sockfd, buf, 50, 0);

				//If the server sends \0, set the break flag to 1
				int break_flag = 0;
				for(int i=0; i<50; i++){
					if(buf[i] == '\0'){
						break_flag = 1;
						break;
					}
				}
				//Print the received packet
				printf("%s", buf);

				//If the server sent \0, break out of the loop
				if(break_flag == 1){
					printf("\n");
					break;
				}
			}
		}
	}
	close(sockfd);
	return 0;

}