
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


//We will receive data in packets of 50 bytes until
//we get a null character or buffer fills up
//Returns the number of bytes received including the null character
//Returns -1 if buffer fills up
int recv_in_packets(int sockfd, char *buffer, size_t buffer_len){
	size_t packet_len = 50;
	char *packet = (char *)malloc(packet_len*sizeof(char));
	int received = 0;
	while(received < buffer_len){
		packet_len = 50;
		for(int i=0; i<packet_len; i++) packet[i] = '\0';
		packet_len = recv(sockfd, packet, packet_len, 0);
		if(packet_len < 0){
			printf("Error in receiving response from server\n");
			close(sockfd);
			exit(0);
		}
		// printf("Received packet of length %d on client\n", packet_len);
		int i;
		for(i=0; i<packet_len; i++){
			if(i+received < buffer_len){
				buffer[i+received] = packet[i];
			}
			else{
				break;
			}
			if(packet[i] == '\0'){
				packet_len = i+1;
				break;
			}
		}
		received += packet_len;
		if(i<packet_len && packet[i] == '\0'){
			break;
		}
	}
	if(received>buffer_len || buffer[received-1] != '\0'){
		received = -1;
	}
	free(packet);
	return received;
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
	
	//Receive the server's greeting
	if(recv(sockfd, buf, 50, 0) < 0){
		printf("Error in receiving response from server\n");
		close(sockfd);
		exit(0);
	}

	//If the server sends LOGIN:, proceed to send the username
	if(strcmp(buf, "LOGIN:") != 0){
		printf("Unrecognised server response\n");
		close(sockfd);
		exit(0);
	}
	printf("%s ", buf);

	//Get the username from the user
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

	//Receive the server's response
	if(recv(sockfd, buf, 50, 0) < 0){
		printf("Error in receiving response from server\n");
		close(sockfd);
		exit(0);
	}

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
	free(buf);

	//Get commands from the user and send them to the server
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
		free(stripped_input);
		free(input);
		//Reallocate to a bigger buffer and receive the response from the server
		buf_len = 1024;
		buf = (char *)malloc(buf_len*sizeof(char));
		if(recv_in_packets(sockfd, buf, buf_len) < 0){
			printf("Error in receiving response from server\n");
			close(sockfd);
			exit(0);
		}

		//If the server sends $$$$ or ####, the command was invalid
		if(strcmp(buf, "$$$$")==0){
			printf("Invalid command\n");
			free(buf);
			continue;
		}
		else if(strcmp(buf, "####")==0){
			printf("Error in running command\n");
			free(buf);
			continue;
		}

		//Print the response from the server
		printf("%s\n", buf);
		free(buf);
	}
	close(sockfd);
	return 0;

}