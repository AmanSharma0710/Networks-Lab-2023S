#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <poll.h>
#include <assert.h>
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
	//Check that n+1 arguments are passed
	assert(argc == NSERVERS+2);

	int			sockfd, newsockfd ;
	int			clilen;
	struct sockaddr_in	cli_addr, serv_addr;

	int i;
    size_t buffer_len = 100;
	char *buf = (char *)malloc(buffer_len*sizeof(char));

	//Create a table to store the servers' loads
	int *loads = (int *)malloc(NSERVERS*sizeof(int));

	//Create a table to store the servers' sockets
	int *sockfds = (int *)malloc(NSERVERS*sizeof(int));

    //Store the servers' information
    struct sockaddr_in *servers = (struct sockaddr_in *)malloc(NSERVERS*sizeof(struct sockaddr_in));
	for(i=0; i<NSERVERS; i++){
		servers[i].sin_family = AF_INET;
		inet_aton("127.0.0.1", &servers[i].sin_addr);
		int port = atoi(argv[i+2]);
		if(port > 0 && port < 65536){
			servers[i].sin_port = htons(port);
		}
		else{
			printf("Invalid port number %d\n", port);
			exit(0);
		}
	}

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Cannot create socket\n");
		exit(0);
	}

	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	int lb_port = atoi(argv[1]);
	if(lb_port > 0 && lb_port < 65536){
		serv_addr.sin_port = htons(lb_port);
	}
	else{
		printf("Invalid port number %d\n", lb_port);
		exit(0);
	}


	/* With the information provided in serv_addr, we associate the server
	   with its port using the bind() system call. 
	*/
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("Unable to bind local address\n");
		exit(0);
	}

	listen(sockfd, 5);

	//Create a poll struct to allow it to wait on sockfd
	struct pollfd poll_set[1];
	poll_set[0].fd = sockfd;
	poll_set[0].events = POLLIN;

	//variable to store the remaining time to wait
	int waitTime = 5;


	//Find and fill the initial load values
	for(i=0; i<NSERVERS; i++){
		if ((sockfds[i] = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			printf("Cannot create socket\n");
			exit(0);
		}
		if (connect(sockfds[i], (struct sockaddr *) &servers[i], sizeof(servers[i])) < 0) {
			printf("Unable to connect to server\n");
			exit(0);
		}
		//Send the message to the server
		strcpy(buf,"Send Load");
		send_in_packets(sockfds[i], buf, buffer_len);
		//Receive the load value from the server
		int received = recv_in_packets(sockfds[i], buf, buffer_len);
		if(received > 0){
			loads[i] = atoi(buf);
			//Print the server IP and load value
			printf("Load value received from server %s %d\n", inet_ntoa(servers[i].sin_addr), loads[i]);
		}
		else{
			printf("Error in receiving load value from server %d\n", i+1);
			exit(0);
		}
		close(sockfds[i]);
	}
	
	//Main loop
	while (1) {
		//We call poll on the accept with a timeout of 5 secs, and if the poll expires
		//due to a client connection we call fork to handle the connection
		clilen = sizeof(cli_addr);
		time_t t = time(NULL);
		int poll_ret = poll(poll_set, 1, waitTime*1000);
		t = time(NULL) - t;
		int time_taken = (int)t;
		waitTime -= time_taken;
		if(poll_ret < 0){
			printf("Error in polling\n");
			exit(0);
		}
		else if(poll_ret == 0){
			//If the poll times out, we connect to all the servers and get updated load values
			for(i=0; i<NSERVERS; i++){
				if ((sockfds[i] = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
					printf("Cannot create socket\n");
					exit(0);
				}
				if (connect(sockfds[i], (struct sockaddr *) &servers[i], sizeof(servers[i])) < 0) {
					printf("Unable to connect to server\n");
					exit(0);
				}
				//Send the message to the server
				strcpy(buf,"Send Load");
				send_in_packets(sockfds[i], buf, buffer_len);
				//Receive the load value from the server
				int received = recv_in_packets(sockfds[i], buf, buffer_len);
				if(received > 0){
					loads[i] = atoi(buf);
					//Print the server IP and load value
					printf("Load value received from server %s %d\n", inet_ntoa(servers[i].sin_addr), loads[i]);
				}
				else{
					printf("Error in receiving load value from server %d\n", i+1);
					exit(0);
				}
				close(sockfds[i]);
			}
			//We then reset the waitTime to 5 secs
			waitTime = 5;
		}
		else{
			//If the poll returns a positive value, we accept the connection
			newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen) ;
			if (newsockfd < 0) {
				printf("Accept error\n");
				exit(0);
			}
			if (fork() == 0) {

				/* This child process will now communicate with the
				client through the send() and recv() system calls.
				*/
				close(sockfd);	/* Close the old socket since all
						communications will be through
						the new socket.
						*/

				//We find the server with the least load
				int min_load = loads[0], min_index = 0;
				for(i=1; i<NSERVERS; i++){
					if(loads[i] < min_load){
						min_load = loads[i];
						min_index = i;
					}
				}
				//We then connect to the server with the least load
				if ((sockfds[min_index] = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
					printf("Cannot create socket\n");
					exit(0);
				}
				if (connect(sockfds[min_index], (struct sockaddr *) &servers[min_index], sizeof(servers[min_index])) < 0) {
					printf("Unable to connect to server\n");
					exit(0);
				}
				//We then send the message to Send Time to the server
				strcpy(buf,"Send Time");

				//We print the server IP we are sending the client request to
				printf("Sending request to server %s\n", inet_ntoa(servers[min_index].sin_addr));

				//We then send the message to the server
				send_in_packets(sockfds[min_index], buf, buffer_len);
				//We then receive the time from the server
				int received = recv_in_packets(sockfds[min_index], buf, buffer_len);
				if(received > 0){
					//We then send the time to the client
					send_in_packets(newsockfd, buf, buffer_len);
				}
				else{
					printf("Error in receiving time from server %d\n", min_index+1);
					exit(0);
				}
				//We then close the connection to the server
				close(sockfds[min_index]);
				//We then close the connection to the client
				close(newsockfd);
				exit(0);
			}
		}
		close(newsockfd);
	}
	return 0;
}
			

