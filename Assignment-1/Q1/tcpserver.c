#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>

int main()
{
	/* Using the template code for an iterative server */
	int			sockfd, newsockfd ;
	int			clilen;
	struct sockaddr_in	cli_addr, serv_addr;

	int i;
	char buf[100];
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Cannot create socket\n");
		exit(0);
	}
	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	serv_addr.sin_port		= htons(20000);
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


		/* We initialize the buffer, copy the current datetime to it,
			and send the message to the client. 
		*/
		
		memset(buf, 0, 100);
		time_t t = time(NULL);
		struct tm tm = *localtime(&t);
		sprintf(buf, "Current server date and time is: %d-%d-%d %d:%d:%d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
		send(newsockfd, buf, strlen(buf) + 1, 0);

		/* We need not wait to receive a message back from the client.
		   We can simply close the socket and go back to waiting for
		   another client request using the original socket descriptor.
		*/ 
		close(newsockfd);
	}
	return 0;
}
			

