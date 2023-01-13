#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>


//We implement a resizable string to store the expression as we do not know its length
struct resizable_string {
	char *str;
	int size;
	int capacity;
};

void init(struct resizable_string *s) {
	s->size = 0;
	s->capacity = 1;
	s->str = malloc(s->capacity);
}

void append(struct resizable_string *s, char c) {
	if (s->size == s->capacity) {
		s->capacity *= 2;
		s->str = realloc(s->str, s->capacity);
	}
	s->str[s->size++] = c;
}

int main()
{
	int			sockfd, newsockfd ; /* Socket descriptors */
	int			clilen;
	struct sockaddr_in	cli_addr, serv_addr;

	int i;
	char buf[100];		/* We will use this buffer for communication */

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
		while(1){
			//Read the expression from the client
			recv(newsockfd, buf, 100, 0);

			//If we read -1\0 we exit
			if(buf[0]=='-' && buf[1]=='1' && buf[2]=='\0'){
				break;
			}

			//We will store the expression in a resizable string and will keep reading till we encounter a \0
			struct resizable_string s;
			init(&s);
			while(1){
				int end = 0;
				for(int i=0; i<100; i++){
					append(&s, buf[i]);
					if(buf[i]=='\0'){
						end = 1;
						break;
					}
				}
				if(end){
					break;
				}
				recv(newsockfd, buf, 100, 0);
			}

			//We will now evaluate the expression
			double ans = 0, ans_bracket=0;
			char op = '+',  op_bracket='+';
			int in_bracket = 0;
			for(int i=0; i<s.size; i++){
				if(s.str[i]=='('){
					in_bracket = 1;
					continue;
				}
				if(s.str[i]==')'){
					in_bracket = 0;
					if(op=='+'){
						ans += ans_bracket;
					}
					else if(op=='-'){
						ans -= ans_bracket;
					}
					else if(op=='*'){
						ans *= ans_bracket;
					}
					else if(op=='/'){
						ans /= ans_bracket;
					}
					ans_bracket = 0;
					continue;
				}
				if(s.str[i]=='+' || s.str[i]=='-' || s.str[i]=='*' || s.str[i]=='/'){
					if(in_bracket){
						op_bracket = s.str[i];
					}
					else{
						op = s.str[i];
					}
					continue;
				}
				if(s.str[i]=='\0'){
					break;
				}
				double num = 0;
				if(s.str[i]>='0' && s.str[i]<='9'){
					num = atof(s.str+i);
					printf("%lf\n", num);
					while((s.str[i]>='0' && s.str[i]<='9')||s.str[i]=='.'){
						i++;
					}
					i--;
				}
				else{
					continue;
				}
				if(in_bracket){
					if(op_bracket=='+'){
						ans_bracket += num;
					}
					else if(op_bracket=='-'){
						ans_bracket -= num;
					}
					else if(op_bracket=='*'){
						ans_bracket *= num;
					}
					else if(op_bracket=='/'){
						ans_bracket /= num;
					}
				}
				else{
					if(op=='+'){
						ans += num;
					}
					else if(op=='-'){
						ans -= num;
					}
					else if(op=='*'){
						ans *= num;
					}
					else if(op=='/'){
						ans /= num;
					}
				}
				printf("%c %c\n", op, s.str[i]);
			}

			//We will now flush the buffer and send the answer to the client
			memset(buf, 0, 100);
			sprintf(buf, "%lf", ans);
			send(newsockfd, buf, 100, 0);
		}

		close(newsockfd);
	}
	return 0;
}
			

