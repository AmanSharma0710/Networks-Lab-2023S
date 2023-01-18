#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
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


int main(){
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
			//users.txt contains a list of usernames
			//Each username is on a new line, followed by a newline character
			char line[50];
			int found = 0;
			while(fgets(line, 50, fp) != NULL){
				if(strcmp(line, buf) == 0){
					found = 1;
					break;
				}
			}
			fclose(fp);

			//Clear the buffer and send the buffer
			for(i=0; i < 50; i++) buf[i] = '\0';
			if(found == 0){
				strcpy(buf, "NOT-FOUND");
				send(newsockfd, buf, strlen(buf) + 1, 0);
				close(newsockfd);
				exit(0);
			}
			else{
				strcpy(buf, "FOUND");
				send(newsockfd, buf, strlen(buf) + 1, 0);
			}

			//We now receive commands from the client that we will execute
			while(1){
				size_t len = 1024;
				char *local_buffer = (char *)malloc(len*sizeof(char));

				//Keep receiving until we get a null character, and add them to local_buffer
				int response_end = 0, local_buffer_filled=0;
				while(response_end == 0){
					recv(newsockfd, buf, 50, 0);
					for(i=0; i < 50; i++){
						local_buffer[local_buffer_filled++] = buf[i];
						if(local_buffer_filled == len){
							printf("Buffer overflow\n");
							exit(0);
						}
						if(buf[i] == '\0'){
							response_end = 1;
							break;
						}
						buf[i] = '\0';
					}
				}
				//Tokenize the command
				char* token = strtok(local_buffer, " ");
				char **tokens = (char **)malloc(3*sizeof(char *));
				int token_count = 0;
				for(int i=0; i<3; i++){
					if(token == NULL){
						break;
					}
					tokens[i] = (char *)malloc(200*sizeof(char));
					strcpy(tokens[i], token);
					token_count++;
					token = strtok(NULL, " ");
				}
				//Clear the local_buffer
				for(i=0; i<1024; i++) local_buffer[i] = '\0';

				//If token_count is 0, it is an empty command and we return empty string
				if(token_count == 0){
					strcpy(buf, "\0");
					send(newsockfd, buf, strlen(buf) + 1, 0);
					free(tokens);
					free(local_buffer);
					continue;
				}

				//Execute the command

				//If the command is "exit", we exit the loop
				if((strcmp(tokens[0], "exit") == 0)&&(token_count == 1)){
					break;	//Exit the loop
				}

				//If the command is "pwd", we return the current working directory
				if((strcmp(tokens[0], "pwd") == 0)&&(token_count == 1)){
					if(getcwd(local_buffer, len) == NULL){	//If we get an error
						strcpy(buf, "####");
						send(newsockfd, buf, strlen(buf) + 1, 0);
					}
					else{	//If we get a valid response
						send_in_packets(newsockfd, local_buffer, strlen(local_buffer));
					}
					free(tokens);
					free(local_buffer);
					continue;
				}

				//If the command is "dir", we return the contents of the current directory if token_count is 1
				//If token_count is 2, we return the contents of the directory specified by the second token
				if((strcmp(tokens[0], "dir") == 0)&&(token_count == 1 || token_count == 2)){
					DIR *d;
					struct dirent *dir;
					if(token_count == 2){
						d = opendir(tokens[1]);
					}
					else if(token_count == 1){
						d = opendir(".");
					}
					if(d){
						while((dir = readdir(d)) != NULL){	//We read the directory and add the contents to local_buffer
							strcat(local_buffer, dir->d_name);
							strcat(local_buffer, "\t");
						}
						closedir(d);
						send_in_packets(newsockfd, local_buffer, strlen(local_buffer));	//We send the contents of local_buffer
					}
					else{	//If the directory could not be opened, we return an error executing command
						strcpy(buf, "####");
						send(newsockfd, buf, strlen(buf) + 1, 0);
					}
					free(tokens);
					free(local_buffer);
					continue;
				}

				//If the command is "cd", we change the current working directory
				//If token_count is 1, we change the current working directory to the path specified by the HOME environment variable
				//If token_count is 2, we change the current working directory to the path specified by the second token
				if((strcmp(tokens[0], "cd") == 0)&&(token_count == 1 || token_count == 2)){
					if(token_count == 1 || strcmp(tokens[1], "~") == 0){	//If the second token is "~", we change the current working directory to the path specified by the HOME environment variable
						if(tokens[1] == NULL)
							tokens[1] = (char *)malloc(200*sizeof(char));
						strcpy(tokens[1], "~/");
					}
					if(tokens[1][0] == '~'){	//If the second token starts with "~", we change the cwd to the path specified by the HOME environment variable
						char *home = getenv("HOME");
						char *temp = (char *)malloc(200*sizeof(char));
						strcpy(temp, home);
						strcat(temp, tokens[1] + 1);
						strcpy(tokens[1], temp);
						//Check for buffer overflow
						if(strlen(tokens[1]) > 200){
							strcpy(buf, "####");
							send(newsockfd, buf, strlen(buf) + 1, 0);
							free(tokens);
							free(local_buffer);
							continue;
						}
						free(temp);
					}
					if(chdir(tokens[1]) == -1){	//If we get an error
						strcpy(buf, "####");
						send(newsockfd, buf, strlen(buf) + 1, 0);
					}
					else{	//If we get a valid response
						strcpy(buf, "Working Directory Changed");
						send(newsockfd, buf, strlen(buf) + 1, 0);
					}
					free(tokens);
					free(local_buffer);
					continue;
				}
				//If we reach here, the command is invalid
				strcpy(buf, "$$$$");
				send(newsockfd, buf, strlen(buf) + 1, 0);
				free(tokens);
				free(local_buffer);
			}
			close(newsockfd);
			exit(0);
		}
		close(newsockfd);
	}
	return 0;
}