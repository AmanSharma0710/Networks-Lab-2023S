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

int NSERVERS = 2;


int main(int argc, char *argv[]){
	int			sockfd, newsockfd ;
	int			clilen;
	struct sockaddr_in	cli_addr;

	int i;
    size_t buffer_len = 100;
	char *buf = (char *)malloc(buffer_len*sizeof(char));

    //Store the server information
    struct sockaddr_in *serv_addr = 

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Cannot create socket\n");
		exit(0);
	}


	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	serv_addr.sin_port		= htons(20000);

	/* With the information provided in serv_addr, we associate the server
	   with its port using the bind() system call. 
	*/
	if (bind(sockfd, (struct sockaddr *) &serv_addr,
					sizeof(serv_addr)) < 0) {
		printf("Unable to bind local address\n");
		exit(0);
	}

	listen(sockfd, 5); /* This specifies that up to 5 concurrent client
			      requests will be queued up while the system is
			      executing the "accept" system call below.
			   */

	/* In this program we are illustrating a concurrent server -- one
	   which forks to accept multiple client connections concurrently.
	   As soon as the server accepts a connection from a client, it
	   forks a child which communicates with the client, while the
	   parent becomes free to accept a new connection. To facilitate
	   this, the accept() system call returns a new socket descriptor
	   which can be used by the child. The parent continues with the
	   original socket descriptor.
	*/
	while (1) {

		/* The accept() system call accepts a client connection.
		   It blocks the server until a client request comes.

		   The accept() system call fills up the client's details
		   in a struct sockaddr which is passed as a parameter.
		   The length of the structure is noted in clilen. Note
		   that the new socket descriptor returned by the accept()
		   system call is stored in "newsockfd".
		*/
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
					&clilen) ;

		if (newsockfd < 0) {
			printf("Accept error\n");
			exit(0);
		}

		/* Having successfully accepted a client connection, the
		   server now forks. The parent closes the new socket
		   descriptor and loops back to accept the next connection.
		*/
		if (fork() == 0) {

			/* This child process will now communicate with the
			   client through the send() and recv() system calls.
			*/
			close(sockfd);	/* Close the old socket since all
					   communications will be through
					   the new socket.
					*/

			/* We initialize the buffer, copy the message to it,
			   and send the message to the client. 
			*/
			
			strcpy(buf,"Message from server");
			send(newsockfd, buf, strlen(buf) + 1, 0);

			/* We again initialize the buffer, and receive a 
			   message from the client. 
			*/
			for(i=0; i < 100; i++) buf[i] = '\0';
			recv(newsockfd, buf, 100, 0);
			printf("%s\n", buf);

			close(newsockfd);
			exit(0);
		}

		close(newsockfd);
	}
	return 0;
}
			

