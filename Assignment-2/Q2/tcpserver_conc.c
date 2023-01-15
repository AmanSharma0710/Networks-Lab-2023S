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
	int			sockfd, newsockfd ; /* Socket descriptors */
	int			clilen;
	struct sockaddr_in	cli_addr, serv_addr;

	int i;
	char buf[50];		/* We will use this buffer for communication */

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Cannot create socket\n");
		exit(0);
	}

	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	serv_addr.sin_port		= htons(20000);

	if (bind(sockfd, (struct sockaddr *) &serv_addr,
					sizeof(serv_addr)) < 0) {
		printf("Unable to bind local address\n");
		exit(0);
	}

	listen(sockfd, 5);

	while (1) {
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
					&clilen) ;

		if (newsockfd < 0) {
			printf("Accept error\n");
			exit(0);
		}

		if (fork() == 0) {
			close(sockfd);	/* Close the old socket since all
					   communications will be through
					   the new socket.
					*/
			
			strcpy(buf,"LOGIN:");
			send(newsockfd, buf, strlen(buf) + 1, 0);

			/* We again initialize the buffer, and receive a 
			   message from the client. 
			*/
			for(i=0; i < 50; i++) buf[i] = '\0';
			recv(newsockfd, buf, 50, 0);

			//We now check the username against usernames stored in a file
			FILE *fp;
			fp = fopen("users.txt", "r");
			char line[50];
			int found = 0;
			while(fgets(line, 50, fp) != NULL){
				if(strcmp(line, buf) == 0){
					found = 1;
					break;
				}
			}
			fclose(fp);

			//Clear the buffer and send the result
			for(i=0; i < 50; i++) buf[i] = '\0';
			if(found == 0){
				strcpy(buf, "NOT-FOUND");
				send(newsockfd, buf, strlen(buf) + 1, 0);
			}
			else{
				strcpy(buf, "FOUND");
				send(newsockfd, buf, strlen(buf) + 1, 0);
			}

			//We now receive commands from the client that we will execute
			while(1){
				for(i=0; i < 50; i++) buf[i] = '\0';
				recv(newsockfd, buf, 50, 0);
				if(strcmp(buf, "exit") == 0){
					break;
				}
				if(getcwd(buf, 50) == NULL){
					strcpy(buf, "ERROR");
					send(newsockfd, buf, strlen(buf) + 1, 0);
				}
				else{
					send(newsockfd, buf, strlen(buf) + 1, 0);
				}
			}


			close(newsockfd);
			exit(0);
		}

		close(newsockfd);
	}
	return 0;
}
			

