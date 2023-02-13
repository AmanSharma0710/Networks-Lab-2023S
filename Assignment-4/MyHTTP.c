#include <string.h>
#include <ctype.h>



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

void receive_in_packets(int sockfd, char *data, size_t data_len){
    size_t packet_len = 50;
    char *packet = (char *)malloc(packet_len*sizeof(char));
    int received = 0;
    while(received < data_len){
        int curr = recv(sockfd, packet, packet_len, 0);
        if(curr == -1){
            perror("recv");
            exit(1);
        }
        received += curr;
    }
    free(packet);
}