#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>

			/* THE SERVER PROCESS */

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


int main(int argc, char *argv[])
{
	int			sockfd, newsockfd ;
	int			clilen;
	struct sockaddr_in	cli_addr, serv_addr;

	int i;
    size_t buffer_len = 50;
	char *buf = (char *)malloc(buffer_len*sizeof(char));

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Cannot create socket\n");
		exit(0);
	}

    //Get port number from the command line arguments
    int port = atoi(argv[1]);
    if(port < 1024 || port > 65535){
        printf("Invalid port number. Port number should be between 1024 and 65535\n");
        exit(0);
    }
	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	serv_addr.sin_port		= htons(port);

	/* With the information provided in serv_addr, we associate the server
	   with its port using the bind() system call. 
	*/
	if (bind(sockfd, (struct sockaddr *) &serv_addr,
					sizeof(serv_addr)) < 0) {
		perror("Unable to bind local address\n");
		exit(0);
	}

	listen(sockfd, 5);

	
	while (1) {
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
					&clilen) ;

		if (newsockfd < 0) {
			perror("Accept error\n");
			exit(0);
		}

		//If we are here, it means that a client has connected to the server
        //Receive the message from the load balancer
        int received = recv_in_packets(newsockfd, buf, buffer_len);
        if(received < 0){
            printf("Error in receiving message from load balancer\n");
            close(newsockfd);
            continue;
        }
        if(strcmp(buf, "Send Load") == 0){
            int load = rand()%100 + 1;
            sprintf(buf, "%d\0", load);
            printf("Load sent: %d\n", load);

            //Send the load to the load balancer
            send_in_packets(newsockfd, buf, strlen(buf));
        }
        else{
            if(strcmp(buf, "Send Time") == 0){
                time_t t = time(NULL);
                struct tm tm = *localtime(&t);
                sprintf(buf, "%d-%d-%d %d:%d:%d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
                
                //Send the time to the load balancer
                send_in_packets(newsockfd, buf, strlen(buf));
            }
            else{
                printf("Invalid message from load balancer\n");
            }
        }
		close(newsockfd);
	}
	return 0;
}
			

