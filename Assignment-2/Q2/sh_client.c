
/*    THE CLIENT PROCESS */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void send_in_packets(int sockfd, char *data, size_t data_len){
	size_t packet_len = 50;
	char *packet = (char *)malloc(packet_len*sizeof(char));
	int sent = 0;
	while(sent < data_len + 1){	// +1 for the null terminator
		int i;
		for(i=0; i<packet_len; i++){
			if(i+sent < data_len){
				packet[i] = data[i+sent];
			}
			else if(i+sent == data_len){
				packet[i] = '\0';
				packet_len = i+1;
			}
			else{
				break;
			}
		}
		send(sockfd, packet, packet_len, 0);
		sent += packet_len;
	}
	free(packet);
}

int main()
{
	int			sockfd ;
	struct sockaddr_in	serv_addr;

	int i;

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

	size_t buf_len = 50;
	char *buf = (char *)malloc(buf_len*sizeof(char));
	for(i=0; i < buf_len; i++) buf[i] = '\0';
	recv(sockfd, buf, buf_len, 0);
	if(strcmp(buf, "LOGIN:") != 0){
		printf("Unrecognised server response\n");
		close(sockfd);
		exit(0);
	}
	printf("%s ", buf);
	size_t input_length = 10;
	char *input = (char *)malloc(input_length*sizeof(char));
	getline(&input, &input_length, stdin);
	if(input_length>26){	//25 + 1 for newline
		printf("Invalid username\n");
		close(sockfd);
		exit(0);
	}
	//Copy the input into the buffer, replacing the newline character with a null character
	for(i=0; i<input_length; i++){
		if(i<input_length-1){
			buf[i] = input[i];
		}else{
			buf[i] = '\0';
		}
	}
	free(input);
	//Send the username to the server
	send(sockfd, buf, 50, 0);
	for(i=0; i < 50; i++) buf[i] = '\0';
	recv(sockfd, buf, 50, 0);
	//If the server sends NOT-FOUND, the username is invalid
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
		input_length = 10;
		input = (char *)malloc(input_length*sizeof(char));
		//Clear the buffer and receive the input from the user
		for(i=0; i < input_length; i++) input[i] = '\0';
		getline(&input, &input_length, stdin);
		input_length = strlen(input);
		input[--input_length] = '\0';	//Remove the newline character from the input
		send_in_packets(sockfd, input, input_length);
		// printf("User entered:%s.\n", input);

		//Strip whitespace from the input to check if the user entered exit
		char *stripped_input = (char *)malloc(input_length*sizeof(char));
		int j=0;
		for(i=0; i<input_length; i++){
			if(input[i] != ' '){
				stripped_input[j++] = input[i];
			}
		}
		stripped_input[j] = '\0';

		//If the user enters exit, close the socket and exit without waiting for a response from the server
		if(strcmp(stripped_input, "exit") == 0){
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
		while(response_end == 0){
			//Clear the buffer and receive the next packet
			for(i=0; i < 50; i++) buf[i] = '\0';
			recv(sockfd, buf, 50, 0);

			//If the server sends \0, set the response_end flag to 1
			for(int i=0; i<50; i++){
				if(buf[i] == '\0'){
					response_end = 1;
					break;
				}
			}
			//Print the received packet
			printf("%s", buf);
		}
		printf("\n");
		free(input);
	}
	close(sockfd);
	return 0;

}