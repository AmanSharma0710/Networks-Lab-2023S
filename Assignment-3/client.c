/*    THE CLIENT PROCESS */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

int main(int argc, char *argv[]){
	int			sockfd ;
	struct sockaddr_in	serv_addr;

	int i;
	size_t len = 100;
	char *buf = (char *)malloc(len*sizeof(char));
	int port = atoi(argv[1]);
	if(port < 0 || port > 65535){
		printf("Invalid port number\n");
		exit(0);
	}

	/* We create the socket and verify its creation */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(0);
	}

	/* We specify the address of the server to which we want to connect */
	serv_addr.sin_family	= AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port	= htons(port);
	if ((connect(sockfd, (struct sockaddr *) &serv_addr,
						sizeof(serv_addr))) < 0) {
		perror("Unable to connect to server\n");
		exit(0);
	}
	
	/* We receive the datetime from the server*/
	int received = recv_in_packets(sockfd, buf, 100);
	if(received < 0){
		printf("Error in receiving response from server\n");
		close(sockfd);
		exit(0);
	}
	//Print the received date and time of the server
	printf("%s\n", buf);
	/* We close the socket after receiving the datetime and printing it*/
	close(sockfd);
	return 0;
}

