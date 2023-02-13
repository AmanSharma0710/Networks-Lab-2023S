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
    ds->data[ds->len] = c;
    ds->data[ds->len+1] = '\0';
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

typedef struct { 
    char* method;
    char* url;
    char* file_path;
    char* serv_ip;
    int serv_port;
    char* file_name; // for PUT request
} Request;

Request* CreateReq(){
    Request* req = (Request*)malloc(sizeof(Request));
    req->method = (char*) malloc(sizeof(char) * MAXLEN);
    req->url = (char*) malloc(sizeof(char) * MAXLEN);
    req->file_path = (char*) malloc(sizeof(char) * MAXLEN);
    req->serv_ip = (char*) malloc(sizeof(char) * MAXLEN);
    req->file_name = (char*) malloc(sizeof(char) * MAXLEN);
    req->serv_port = 80;
    return req;
}

void DelReq(Request* req){
    free(req->method);
    free(req->url);
    free(req->file_path);
    free(req->file_name);
    free(req->serv_ip);
    free(req);
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

void receive_headers(int sockfd, dynamic_string* ds){
    empty_string(ds);
    char buffer[50];
    int received = 0;
    while(1){
        int n = recv(sockfd, buffer, 50, MSG_PEEK);
        if(n<0){
            perror("Error in receiving");
            exit(1);   
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
}

void receive_content(int sockfd, dynamic_string* ds, int content_len){
    empty_string(ds);
    char buffer[50];
    int received = 0;
    while(received < content_len){
        int n = recv(sockfd, buffer, 50, 0);
        if(n < 0){
            perror("Error in receiving");
            exit(1);
        }
        received += n;
        for(int i=0; i<n; i++){
            insert_into_string(ds, buffer[i]);
        }
    }
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
    while(*content_type != '\r'){
        type[i++] = *content_type;
        content_type++;
    }
    type[i] = '\0';
    return type;
}

int parse_request(char* command, Request* request){
    // printf("command: %s\n", command);
    // first we parse the command to get the method and url
    char* method = strtok(command, " ");
    if (strcmp(method, "GET") == 0 || strcmp(method, "PUT") == 0){
        strcpy(request->method, method);
    }
    else{
        printf("Invalid method\n");
        return -1;
    }
    // printf("method: %s\n", request->method);
    
    char* url = strtok(NULL, " ");
    if (url == NULL){
        printf("No URL\n");
        return -1;
    }
    strcpy(request->url, url);
    // printf("url: %s\n", request->url);


    // if the method is PUT, we need to parse the file name
    if (strcmp("PUT", request->method) == 0){
        char* file_name = strtok(NULL, " ");
        if (file_name == NULL){
            printf("No file name\n");
            return -1;
        }
        strcpy(request->file_name, file_name);
        // printf("file_name: %s",file_name);
    }
    else{
        char* file_name = strtok(NULL, " ");
        if (file_name != NULL){
            printf("Invalid command\n");
            return -1;
        }
    }

    // now we parse the url to get the server ip and port
    // url format will be http://<ip>/<path>:<port>
    char* ip = strtok(url, "/");
    if (strcmp(ip, "http:") != 0){
        printf("Invalid url\n");
        return -1;
    }
    ip = strtok(NULL, "/");
    strcpy(request->serv_ip, ip);
    // printf("ip: %s\n", request->serv_ip);

    char* path = strtok(NULL, ":");
    strcpy(request->file_path, "/");
    if (path != NULL){
        strcat(request->file_path, path);
    }
    // printf("path: %s\n", request->file_path);

    char* port = strtok(NULL, ":");
    if (port == NULL){
        request->serv_port = SERV_PORT;
    }
    else{
        request->serv_port = atoi(port);
    }
    // printf("port: %d\n", request->serv_port);
}

// this function opens a tcp/ip connection with the given url/server
int Connect(Request* request){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        printf("Error in opening socket\n");
        return -1;
    }
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(request->serv_port);
    serv_addr.sin_addr.s_addr = inet_addr(request->serv_ip);
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
        printf("Error in connecting to server\n");
        return -1;
    }
    return sockfd;
}

// This function sends the GET request to the server and receives the response
void Process_GET(Request* request){
    // opens tcp connection with the http server
    int sockfd = Connect(request);
    printf("Connected to server\n");
    // download/retrieve the document from the http server
    dynamic_string* req = create_dynamic_string();
    char* temp = (char*)malloc(sizeof(char)*MAXLEN);
    sprintf(temp,"GET %s HTTP/1.1\r\nHost: %s\r\n", request->file_path, request->serv_ip);
    
    insert_mess_into_string(req, temp);
    // connection
    insert_mess_into_string(req,"Connection: close\r\n");
    
    // date 
    time_t tnull = time(NULL);
    struct tm* local;
    local = localtime(&tnull);
    char *tme = asctime(local);
    insert_mess_into_string(req,"Date: ");
    insert_mess_into_string(req,tme);
    insert_mess_into_string(req,"\r\n");

    // accept
    insert_mess_into_string(req,"Accept: ");
    insert_mess_into_string(req,get_mime_type(request->file_path));
    insert_mess_into_string(req,"\r\n");
    
    // Accept-Language: en-us preferred, otherwise just English
    insert_mess_into_string(req,"Accept-Language: en-us, en\r\n");

    // if-modified since
    insert_mess_into_string(req, "If-Modified-Since: ");
    // get current time - 2 days
    local->tm_mday -= 2;
    mktime(local);
    char *time2= asctime(local);
    insert_mess_into_string(req,time2);
    insert_mess_into_string(req,"\r\n");

    // end of headers
    insert_mess_into_string(req,"\r\n");

    // send the request to the server
    printf("Request:\n%s\n",req->data);
    send_in_packets(sockfd, req->data, req->len);
    
    // receive the response from the server
    // we also add a timeout of 3 seconds. If we dont receive the data within
    // the timeout we close the connection and print the timeout message
    int content_len;
    dynamic_string* content, *response;
    struct pollfd fds[1];
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;
    int ret = poll(fds, 1, 3000);

    if (ret < 0){
        printf("Error in polling\n");
        return;
    }
    else if (ret == 0){
        printf("Timeout\n");
        return;
    }
    else{
        if (fds[0].revents & POLLIN){
            response = create_dynamic_string();
            receive_headers(sockfd,response);

            content_len = get_content_length(response->data);
            // We can parse the headers here but since we are not using all of them
            // we have only parsed the content length
            content = create_dynamic_string();
            receive_content(sockfd, content, content_len);
        }
    } 
    // close the connection
    close(sockfd);
    free(temp);
    // we get the status code from the response
    temp = strtok(response->data, " ");
    temp = strtok(NULL, " ");
    int status_code = atoi(temp);

    if (status_code != 200){
        if (status_code == 404){
            printf("Error 404: File not found\n");
        }
        else if (status_code == 400){
            printf("Error 400: Bad request\n");
        }
        else if (status_code == 403){
            printf("Error 403: Forbidden\n");
        }
        else {
            printf("Error %d: Unknown error\n", status_code);
        }
        return;
    }
    
    // get the name of the file from the request->file_path
    // get the last token splitting request->file_path with delim = /
    char* filename = strrchr(request->file_path, '/');
    if (filename != NULL) filename = filename + 1;
    else{
        printf("Error in file_path\n");
        exit(1);
    }
    FILE* fptr = fopen(filename,"w");
    if (fptr == NULL){
        printf("Error in opening file\n");
        exit(1);
    }
    // now we write to the file 
    for(int i=0;i<content->len;i++){
        fputc(content->data[i],fptr);
    }

    // get the content type header from the response
    char* content_type = get_content_type(response->data);

    int pid = fork();
    if (pid == 0){
        // child process
        // the file should be opened in the application according to the following legend:
        // .html -> chrome
        // .pdf -> adobe acrobat
        // .jpeg -> gimp
        // any other -> gedit
        if (strcmp(content_type, "text/html") == 0){
            execlp("google-chrome", "google-chrome", filename, NULL);
        }
        else if (strcmp(content_type, "application/pdf") == 0){
            execlp("acroread", "acroread", filename, NULL);
        }
        else if (strcmp(content_type, "image/jpeg") == 0){
            execlp("gimp", "gimp", filename, NULL);
        }
        else{
            execlp("gedit", "gedit", filename, NULL);
        }

    }
    return;
}

void Process_PUT(Request* request){
    // opens tcp connection with the http server
    int sockfd = Connect(request);

    // upload the document to the http server
    dynamic_string* req = create_dynamic_string();
    char* temp = (char*)malloc(sizeof(char)*MAXLEN);
    sprintf(temp,"PUT %s HTTP/1.1\r\nHost: %s\r\n", request->file_path, request->serv_ip);
    insert_mess_into_string(req, temp);

    // connection
    insert_mess_into_string(req,"Connection: close\r\n");
    
    // date 
    time_t tnull = time(NULL);
    struct tm* local;
    local = localtime(&tnull);
    char *tme = asctime(local);
    insert_mess_into_string(req,"Date: ");
    insert_mess_into_string(req,tme);
    insert_mess_into_string(req,"\r\n");

    // accept
    insert_mess_into_string(req,"Accept: ");
    insert_mess_into_string(req,get_mime_type(request->file_path));
    insert_mess_into_string(req,"\r\n");

    // Accept-Language: en-us preferred, otherwise just English
    insert_mess_into_string(req,"Accept-Language: en-us, en\r\n");

    // if-modified-since
    insert_mess_into_string(req, "If-Modified-Since: ");
    // get current time - 2 days
    local->tm_mday -= 2;
    mktime(local);
    char *time2= asctime(local);
    insert_mess_into_string(req,time2);
    insert_mess_into_string(req,"\r\n");

    // content-language
    insert_mess_into_string(req,"Content-Language: en-us\r\n");

    // read the file and get the content length
    FILE* fptr = fopen(request->file_name,"r");
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
    insert_mess_into_string(req,get_mime_type(request->file_path));
    insert_mess_into_string(req,"\r\n");

    // end of headers
    insert_mess_into_string(req,"\r\n");

    // read the file and get the content
    fptr = fopen(request->file_name,"r");
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
    printf("Request:\n%s\n",req->data);
    send_in_packets(sockfd,req->data,req->len);

    // receive the response
    dynamic_string* response = create_dynamic_string();
    // receive the headers
    receive_headers(sockfd,response);

    close(sockfd);

    free(temp);

    // get the status code

    temp = strtok(response->data, " ");
    temp = strtok(NULL, " ");
    int status_code = atoi(temp);

    if (status_code != 200){
        if (status_code == 404){
            printf("Error 404: File not found\n");
        }
        else if (status_code == 400){
            printf("Error 400: Bad request\n");
        }
        else if (status_code == 403){
            printf("Error 403: Forbidden\n");
        }
        else {
            printf("Error %d: Unknown error\n", status_code);
        }
        return;
    }    
}

int main(){
    while(1){
        printf("MyBrowser> ");
        char* command = (char*)malloc(100*sizeof(char));
        size_t size = 100;
        int n = getline(&command,&size,stdin);
        command[n-1] = '\0';
        Request* request = CreateReq();
        if (strcmp(command, "QUIT") == 0){
            printf("Browser is shutting down\n");
            break;
        }
        if (parse_request(command, request) == -1) continue;
        if (strcmp(request->method, "GET") == 0){
            printf("Processing GET request\n");
            Process_GET(request);
        } else if (strcmp(request->method, "PUT") == 0){
            printf("Processing PUT request\n");
            Process_PUT(request);
        }
        free(command);
        DelReq(request);
    }
    return 0;
}