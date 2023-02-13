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

typedef struct { 
    char* method;
    char* url;
    char* file_path;
    char* serv_ip;
    int serv_port;
    char* file_name;
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

    // download/retrieve the document from the http server
    char* req;
    sprintf(req,"GET %s HTTP/1.1\r\nHost: %s\r\n", request->file_path, request->serv_ip);
    
    // connection
    strcat(req,"Connection: close\r\n");
    
    // date 
    time_t tnull = time(NULL);
    struct tm* local;
    local = localtime(&tnull);
    char *tme = asctime(local);
    strcat(req,"Date: ");
    strcat(req,tme);
    strcat(req,"\r\n");

    // accept
    // TODO: extension function

    // Accept-Language: en-us preferred, otherwise just English
    // TODO: just english how to manage
    strcat(req,"Accept-Language: en-us\r\n");

    // if-modified since
    strcat(req, "If-Modified-Since: ");
    // get current time - 2 days
    local->tm_mday -= 2;
    mktime(local);
    char *time2= asctime(local);
    strcat(req,time2);
    strcat(req,"\r\n");

    // end of headers
    strcat(req,"\r\n");

    // send the request to the server
    printf("%s\n",req);
    send_in_packets(sockfd, req, strlen(req));
    
    // receive the response from the server
    // we also add a timeout of 3 seconds. If we dont receive the data within
    // the timeout we close the connection and print the timeout message


    // open the received file

    // close the connection 
    close(sockfd);
}

void Process_PUT(Request* request){
    
    
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