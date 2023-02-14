#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<poll.h> 
#include<string.h>
#include<sys/types.h>
#include<time.h>
#include<sys/socket.h>
#include <arpa/inet.h>
#include<netinet/in.h>
#include<ctype.h>
#include<sys/stat.h>

#define SERV_PORT 80
#define MAXLEN 1000

typedef struct{
    char *data;
    int len;
    int size;
} dynamic_string;

dynamic_string* create_dynamic_string(){
    dynamic_string* ds = (dynamic_string*)malloc(sizeof(dynamic_string));
    ds->data = (char *)malloc(sizeof(char));
    ds->data[0] = '\0';
    ds->len = 0;
    ds->size = 1;
    return ds;
}

void insert_into_string(dynamic_string* ds, char c){
    if(ds->len == ds->size-1){
        ds->data = (char *)realloc(ds->data, ds->size*2);
        ds->size *= 2;
    }
    ds->data[ds->len++] = c;
    ds->data[ds->len] = '\0';
}

void insert_mess_into_string(dynamic_string* ds, char* mess){
    int len = strlen(mess);
    for(int i=0; i<len; i++){
        insert_into_string(ds, mess[i]);
    }
}

void delete_string(dynamic_string* ds){
    free(ds->data);
    free(ds);
}

void empty_string(dynamic_string* ds){
    free(ds->data);
    ds->data = (char *)malloc(sizeof(char));
    ds->data[0] = '\0';
    ds->len = 0;
}

char *str_tolower(char *str){
    char *p = str;
    while (*p) {
        *p = tolower(*p);
        p++;
    }
    return str;
}

char* get_mime_type(char* filename){
    char* ext = strrchr(filename, '.');
    if (ext == NULL) {
        return "text/*";
    }
    ext++;
    ext = str_tolower(ext);
    if (strcmp(ext, "html") == 0) {
        return "text/html";
    }
    if(strcmp(ext, "pdf") == 0){
        return "application/pdf";
    }
    if(strcmp(ext, "jpg") == 0){
        return "image/jpeg";
    }
    return "text/*";
}

void send_in_packets(int sockfd, char *data, size_t data_len){
	size_t packet_len = 50;
	char *packet = (char *)malloc(packet_len*sizeof(char));
	int sent = 0;
	while(sent < data_len){
		int i;
		for(i=0; i<packet_len; i++){
			if(i+sent < data_len){
				packet[i] = data[i+sent];
			}
			else{
                packet_len = i;
				break;
			}
		}
		send(sockfd, packet, packet_len, 0);
		sent += packet_len;
	}
	free(packet);
}

int receive_headers(int sockfd, dynamic_string* ds){
    empty_string(ds);
    char buffer[50];
    int received = 0;
    while(1){
        int n = recv(sockfd, buffer, 50, MSG_PEEK);
        if(n<0){
            perror("Error in receiving");
            exit(1);   
        }
        if(n==0){
            printf("Connection unexpectedly closed by server\n");
            return 1;
        }
        int to_receive = n, flag = 0;
        for(int i=0; i<n; i++){
            insert_into_string(ds, buffer[i]);
            if(ds->len>=4){
                if(ds->data[ds->len-4] == '\r' && ds->data[ds->len-3] == '\n' && ds->data[ds->len-2] == '\r' && ds->data[ds->len-1] == '\n'){
                    to_receive = i+1;
                    flag = 1;
                    break;
                }
            }
        }
        recv(sockfd, buffer, to_receive, 0);
        received += to_receive;
        if(flag) break;
    }
    return 0;
}

int receive_content(int sockfd, dynamic_string* ds, int content_len){
    empty_string(ds);
    char buffer[50];
    int received = 0;
    while(received < content_len){
        int n = recv(sockfd, buffer, 50, 0);
        if(n < 0){
            perror("Error in receiving");
            exit(1);
        }
        if(n == 0){
            printf("Connection unexpectedly closed by server\n");
            return 1;
        }
        for(int i=0; i<n; i++){
            insert_into_string(ds, buffer[i]);
            received++;
            if(received == content_len) break;
        }
    }
    return 0;
}

// find the content length from the header
int get_content_length(char* headers){
    char* content_length = strstr(headers, "Content-Length: ");
    if(content_length == NULL) return -1;
    content_length += 16;
    int len = 0;
    while(*content_length != '\r'){
        len = len*10 + (*content_length - '0');
        content_length++;
    }
    return len;
}

char* get_content_type(char* headers){
    char* content_type = strstr(headers, "Content-Type: ");
    if(content_type == NULL) return NULL;
    content_type += 14;
    char* type = (char*)malloc(sizeof(char)*MAXLEN);
    int i = 0;
    while(*content_type != '\r' && *content_type != ';'){
        type[i++] = *content_type;
        content_type++;
    }
    type[i] = '\0';
    return type;
}

int parse_request(char* request){
    char *req = strcpy((char*)malloc(sizeof(char)*strlen(request)+1), request);
    char* method = strtok(req, " ");
    if(strcmp(method, "GET") == 0){
        free(req);
        return 1;
    }
    if(strcmp(method, "PUT") == 0){
        free(req);
        return 2;
    }
    free(req);
    return 0;
}

void send_error_response(int sockfd, int errcode){
    if(errcode == 200){
        char* response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";
        send_in_packets(sockfd, response, strlen(response));
    }
    else if(errcode == 400){
        char* response = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";
        send_in_packets(sockfd, response, strlen(response));
    }
    else if(errcode == 403){
        char* response = "HTTP/1.1 403 Forbidden\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";
        send_in_packets(sockfd, response, strlen(response));
    }
    else{
        char* response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";
        send_in_packets(sockfd, response, strlen(response));
    }
    return;
}


void handle_get_request(int sockfd, char* request){
    // get the file path
    char* file_path = strtok(request, " ");
    file_path = strtok(NULL, " ");

    dynamic_string* req = create_dynamic_string();
    char* temp = (char*)malloc(sizeof(char)*MAXLEN);
    sprintf(temp,"HTTP/1.1 200 OK\r\n");
    insert_mess_into_string(req, temp);

    // expires
    insert_mess_into_string(req, "Expires: ");
    // get current time + 3 days
    time_t now = time(0);
    now += 3*24*60*60;
    struct tm tm = *gmtime(&now);
    memset(temp, 0, MAXLEN);
    strftime(temp, MAXLEN, "%a, %d %b %Y %H:%M:%S %Z", &tm);
    insert_mess_into_string(req, temp);
    insert_mess_into_string(req,"\r\n");

    // content-language
    insert_mess_into_string(req,"Cache-control: no-store\r\n");

    // content-language
    insert_mess_into_string(req,"Content-Language: en-us\r\n");

    //check if we have access to the file
    // if we don't have access, send 403
    if(access(file_path, F_OK) == -1){
        // file doesn't exist
        send_error_response(sockfd, 403);
        return;
    }
    
    // last-modified
    struct stat attr;
    stat(file_path, &attr);
    tm = *gmtime(&(attr.st_mtime));
    memset(temp, 0, MAXLEN);
    strftime(temp, MAXLEN, "%a, %d %b %Y %H:%M:%S %Z", &tm);
    insert_mess_into_string(req,"Last-Modified: ");
    insert_mess_into_string(req,temp);
    insert_mess_into_string(req,"\r\n");

    // read the file and get the content length
    FILE* fptr = fopen(file_path,"r");
    if (fptr == NULL){
        printf("Error in opening file\n");
        exit(1);
    }
    fseek(fptr, 0, SEEK_END);
    int content_len = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);
    fclose(fptr);

    // content-length
    insert_mess_into_string(req,"Content-Length: ");
    free(temp);
    temp = (char*)malloc(sizeof(char)*MAXLEN);
    sprintf(temp,"%d\r\n",content_len);
    insert_mess_into_string(req,temp);
    
    // content-type
    insert_mess_into_string(req,"Content-Type: ");
    insert_mess_into_string(req,get_mime_type(file_path));
    insert_mess_into_string(req,"\r\n");

    // end of headers
    insert_mess_into_string(req,"\r\n");

    // read the file and get the content
    fptr = fopen(file_path,"r");
    if (fptr == NULL){
        printf("Error in opening file\n");
        exit(1);
    }
    char* content = (char*)malloc(content_len*sizeof(char));
    fread(content,1,content_len,fptr);
    fclose(fptr);

    // add the content to the request
    insert_mess_into_string(req,content);

    // send the request in packets
    send_in_packets(sockfd,req->data,req->len);
    return;
}

void handle_put_request(int sockfd, char* request){
    // get the file path
    char* file_path = strcpy((char*)malloc(sizeof(char)*MAXLEN), request);
    file_path = strtok(file_path, " ");
    file_path = strtok(NULL, " ");

    // check if the directory exists
    char* dir_path = (char*)malloc(sizeof(char)*MAXLEN);
    strcpy(dir_path,file_path);
    char* temp = strrchr(dir_path,'/');
    *temp = '\0';
    struct stat st;
    if(stat(dir_path, &st) == -1){
        // directory doesn't exist
        // make the directory
        mkdir(dir_path, 0777);
    }

    // get the content length
    int len = get_content_length(request);
    if(len == -1){
        // content length not found
        send_error_response(sockfd, 400);
        return;
    }

    // get the content
    dynamic_string* content = create_dynamic_string();
    if(receive_content(sockfd, content, len) == 1){
        // error in receiving content
        send_error_response(sockfd, 400);
        return;
    }

    // write the content to the file
    FILE* fptr = fopen(file_path,"w");
    if (fptr == NULL){
        send_error_response(sockfd, 403);
        return;
    }

    fwrite(content->data,1,content->len,fptr);
    fclose(fptr);

    // send the response
    send_error_response(sockfd, 200);

    // free the memory
    free(dir_path);
    delete_string(content);
    return;
}


int main(){
    //Setup the server
    int sockfd = socket(AF_INET, SOCK_STREAM, 0), newsockfd;
    if(sockfd < 0){
        perror("Error in creating socket");
        exit(1);
    }
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    if(bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
        perror("Error in binding");
        exit(1);
    }
    if(listen(sockfd, 10) < 0){
        perror("Error in listening");
        exit(1);
    }
    //Now we are ready to accept connections
    while(1){
        newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
        if(newsockfd < 0){
            perror("Error in accepting");
            exit(1);
        }
        //Now we have a connection
        //We need to fork a child process to handle the connection
        int pid = fork();
        if(pid < 0){
            perror("Error in forking");
            exit(1);
        }
        if(pid == 0){
            //This is the child process
            close(sockfd);
            //We need to handle the connection
            //First we need to receive the request
            dynamic_string* request = create_dynamic_string();
            if(receive_headers(newsockfd, request) == 1){
                //Error in receiving headers
                send_error_response(newsockfd, 400);
                //Now we need to close the connection
                close(newsockfd);
                //Now we need to free the memory
                delete_string(request);
                exit(0);
            }
            printf("%s", request->data);

            //Now we need to parse the request
            int req_type = parse_request(request->data);
            if(req_type == 1){
                //This is a GET request
                //We need to handle it
                handle_get_request(newsockfd, request->data);
            }
            else if(req_type == 2){
                //This is a PUT request
                //We need to handle it
                handle_put_request(newsockfd, request->data);
            }
            else{
                //This is an invalid request
                //We need to send a 400 Bad Request response
                send_error_response(newsockfd, 400);
            }
            //Now we need to close the connection
            close(newsockfd);
            //Now we need to free the memory
            delete_string(request);
            exit(0);
        }
        else{
            //This is the parent process
            //We need to close the connection
            close(newsockfd);
        }
    }
    return 0;
}