/*
    Ashish Rekhani 20CS10010
    Aman Sharma 20CS30063
    To compile: gcc -o pingnetinfo pingnetinfo.c
    To run: sudo ./pingnetinfo <site> <n> <T>
*/

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<netdb.h>
#include<string.h>
#include<netinet/udp.h>
#include<netinet/ip_icmp.h>
#include<netinet/ip.h>
#include<sys/time.h>
#include<sys/select.h>
#include<netinet/tcp.h>
#include<poll.h>

#define MAXSITELEN 1000
#define NSIZES 10
#define TRIES 3

double prev_slope = 0.0;

void printpacket(struct ip *ip){
    struct icmphdr* icmph = (struct icmphdr*)(ip + ip->ip_hl*4);
    printf("ICMP packet:\n");
    printf("\tType: %d\n", icmph->type);
    printf("\tCode: %d\n", icmph->code);
    printf("\tChecksum: %d\n", icmph->checksum);
    printf("\tID: %d\n", icmph->un.echo.id);
    printf("\tSequence: %d\n", icmph->un.echo.sequence);

    // check for non echo request/reply or time exceeded packets

    if (icmph->type != ICMP_ECHOREPLY && icmph->type != ICMP_ECHO && icmph->type != ICMP_TIME_EXCEEDED){
        printf("Not an echo request/reply or time exceeded packet\n");
        printf("\tData:\n");
        struct iphdr* iph = (struct iphdr*)(icmph + sizeof(struct icmphdr));
        printf("\t\tVersion: %d\n", iph->version);
        printf("\t\tHeader length: %d\n", iph->ihl);
        printf("\t\tType of service: %d\n", iph->tos);
        printf("\t\tTotal length: %d\n", iph->tot_len);
        printf("\t\tIdentification: %d\n", iph->id);
        printf("\t\tFragment offset: %d\n", iph->frag_off);
        printf("\t\tTTL: %d\n", iph->ttl);

        if (iph->protocol == IPPROTO_TCP){
            printf("\t\tProtocol: TCP\n");
            struct tcphdr* tcph = (struct tcphdr*)(iph + iph->ihl*4);
            printf("\t\tSource port: %d\n", tcph->source);
            printf("\t\tDestination port: %d\n", tcph->dest);
            printf("\t\tSequence number: %d\n", tcph->seq);
            printf("\t\tAcknowledgement number: %d\n", tcph->ack_seq);
            printf("\t\tData offset: %d\n", tcph->doff);
            printf("\t\tWindow: %d\n", tcph->window);
            printf("\t\tChecksum: %d\n", tcph->check);
            printf("\t\tFlags: \n");
            printf("\t\t\tURG: %d\n", tcph->urg);
            printf("\t\t\tACK: %d\n", tcph->ack);
            printf("\t\t\tPSH: %d\n", tcph->psh);
            printf("\t\t\tRST: %d\n", tcph->rst);
            printf("\t\t\tSYN: %d\n", tcph->syn);
            printf("\t\t\tFIN: %d\n", tcph->fin);
        }
        else if (iph->protocol == IPPROTO_UDP){
            printf("\t\tProtocol: UDP\n");
            struct udphdr* udph = (struct udphdr*)(iph + iph->ihl*4);
            printf("\t\tSource port: %d\n", udph->source);
            printf("\t\tDestination port: %d\n", udph->dest);
            printf("\t\tLength: %d\n", udph->len);
            printf("\t\tChecksum: %d\n", udph->check);
        }
        else{
            printf("\t\tUnknown Protocol: %d\n", iph->protocol);
        }
    }
}

unsigned short checksum(unsigned short *buf, int packet_size){
    unsigned short answer = 0;
    unsigned short sum = 0;

    unsigned short *w = buf;
    // convert the packet to 16 bit words
    packet_size += sizeof(struct icmphdr);
    int nleft = packet_size;
    while(nleft > 1){
        sum += *w++;
        nleft -= 2;
    }
    if (nleft == 1){
        sum += answer;
    }
    sum = sum>>16 + sum & 0xffff;
    sum += sum>>16;
    answer = ~sum;
    return answer;
}

void latency_bandwidth_function(char* inter_ip, int n, int T, int sock_fd, int ttl, double bandwidth_ret[], double latency_ret[]){
    // this function will estimate the latency and bandwidth of the network of the intermediate link
    // by sending n packets each at T seconds interval for different packet sizes
    // and return the average latency of the network and the bandwidth of the network

    // we will now decide different sizes of packets to figure the bandwidth
    int size_packets[NSIZES];
    int size_f = 64;
    int size_l = 4096;
    for(int i = 0; i < NSIZES; i++){
        size_packets[i] = size_f + i * (size_l - size_f) / (NSIZES - 1);
    }

    struct sockaddr_in inter_addr;
    double rtts[NSIZES];
    for(int i = 0; i < NSIZES; i++){
        rtts[i] = 10000.0;
    }

    // we will be using the same socket for all the packets
    for(int nsize = 0; nsize < NSIZES; nsize++){
        int packet_size = size_packets[nsize];
        char* send_packet = (char*) malloc(packet_size*sizeof(char));
        memset(send_packet, 0, packet_size); 
        // creating the ICMP packet
        struct icmp *icmp = (struct icmp *)send_packet;
        icmp->icmp_type = ICMP_ECHO;
        icmp->icmp_seq = 0;
        icmp->icmp_cksum = 0;
        icmp->icmp_code = 0;
        // icmp->icmp_seq = 0;
        // icmp->icmp_cksum = checksum((unsigned short *)icmp, packet_size);

        // create a table to store the sent and recv time of each packet
        struct timeval sent_time[n];
        struct timeval recv_time[n];
        for(int i=0;i<n;i++){
            sent_time[i].tv_sec = 0;
            sent_time[i].tv_usec = 0;
            recv_time[i].tv_sec = 0;
            recv_time[i].tv_usec = 0;
        }
        int tries = 0;
        // for each size of packet, we will send n packets
        for(int nprobe = 0; nprobe < n; nprobe++){
            // we will be using different id numbers for different packets of same size
            // so that we can differentiate between them and calculate the rtt for each packet irrespective of the order
            // in which they are received
            icmp->icmp_id = nprobe + 1;
            icmp->icmp_cksum = checksum((unsigned short *)icmp, packet_size+sizeof(struct icmphdr));
            inter_addr.sin_family = AF_INET;
            inter_addr.sin_addr.s_addr = inet_addr(inter_ip);
            struct pollfd pfd;
            pfd.fd = sock_fd;
            pfd.events = POLLIN;

            // send the packet
            gettimeofday(&sent_time[nprobe], NULL);
            int send_status = sendto(sock_fd, send_packet, packet_size, 0, (struct sockaddr *)&inter_addr, sizeof(inter_addr));
            if(send_status < 0){
                perror("Error in sending packet: ");
                exit(1);
            }
            
            char* recv_packet = (char*) malloc(packet_size*sizeof(char));
            memset(recv_packet, 0, packet_size);
            memset(&inter_addr, 0, sizeof(inter_addr));
            socklen_t inter_addr_len = sizeof(inter_addr);
            int poll_status = poll(&pfd, 1, 1000);
            if (poll_status < 0){
                perror("Error in polling: ");
                exit(1);
            }
            else if (poll_status == 0){
                if (tries < TRIES){
                    printf("Packet number %d timed out on try number %d\n", nprobe + 1, tries + 1);
                    printf("Retrying...\n");
                    tries++;
                    nprobe--;
                    continue;
                }
                else{
                    printf("Packet number %d failed to be received 3 times. We skip this node and move to the next node\n\n", nprobe + 1);
                    bandwidth_ret[ttl] = bandwidth_ret[ttl - 1];
                    latency_ret[ttl] = latency_ret[ttl - 1];
                    return;
                }
            }
            
            int recv_status = recvfrom(sock_fd, recv_packet, packet_size, 0, (struct sockaddr *)&inter_addr, &inter_addr_len);
            if(recv_status < 0){
                perror("Error in receiving packet: ");
                exit(1);
            }
            
            // check the id number of the received packet
            struct icmp *icmp_recv = (struct icmp *)recv_packet;
            // store the recv time of the packet
            int id_no = icmp_recv->icmp_id;
            printf("Received packet with id no: %d\n", id_no);
            if (id_no > NSIZES || id_no < 1){
                printf("Packet with invalid id number received\n");
                // if the id number is invalid, we will not consider this packet
                printpacket((struct ip*)recv_packet);
                nprobe--;
                continue;
            }
            tries = 0;
            gettimeofday(&recv_time[id_no - 1], NULL);
            free(recv_packet);
            usleep(T*1000);
        }

        // calculate the rtt for each packet and store it in the table
        for(int i = 0; i < n; i++){
            double rtt = (recv_time[i].tv_sec - sent_time[i].tv_sec) * 1000.0;
            rtt += (recv_time[i].tv_usec - sent_time[i].tv_usec) / 1000.0;
            if(rtt < rtts[nsize]){
                rtts[nsize] = rtt;
            }
        }
        free(send_packet);
    }

    for(int i = 0; i < NSIZES; i++){
        printf("Minimum rtt for size %d is %f ms\n", size_packets[i], rtts[i]);
    }

    double avg_slope = 0.0;
    for(int i = 0; i < NSIZES - 1; i++){
        double slope = (rtts[i + 1] - rtts[i]) / (size_packets[i + 1] - size_packets[i]);
        avg_slope += slope;
    }
    avg_slope /= (NSIZES - 1);
    printf("Average slope is %f ms/byte\n", avg_slope);

    // calculate the bandwidth
    double bandwidth_inv = avg_slope - prev_slope;
    // bandwidth is in Mbps and bandwidth_inv is in ms/byte so convert it to Mbps
    double bandwidth = (2000*8)/(bandwidth_inv*1024*1024);
    prev_slope = bandwidth_inv;

    if (bandwidth < 0){
        bandwidth = 0.0001;
    }
    printf("Bandwidth for the link is %f Mbps\n", bandwidth);
    
    bandwidth_ret[ttl] = bandwidth;

    double latency = 0.0;
    for(int i = 0; i < NSIZES; i++){
        latency += rtts[i] - 2*(size_packets[i]*8000.0/bandwidth*1024*1024);
        for(int j = 1;j<ttl;j++){
            latency -= 2*(size_packets[i]*8000.0/bandwidth_ret[j]*1024*1024) + latency_ret[j];
        }
    }
    latency /= NSIZES;
    if (latency < 0){
        latency = 0.0;
        printf("Latency calculated is negative. Setting it to 0\n");
    }
    else printf("Average Latency for the link is %f ms\n", latency); 
    latency_ret[ttl] = latency;   
}

int main(int argc, char* argv[]){
    if (argc != 4){
        printf("Usage: ./traceroute <site name or IP address> <number of probes> <time difference in miliseconds>");
        exit(1);
    }

    char* site = argv[1];
    size_t len = 0;
    // printf("Enter the site name or IP address: ");
    // int sizesite = getline(&site, &len, stdin);
    // remove the newline character
    // site[sizesite - 1] = '\0';
    // printf("Input site or IP address: %s\n", site);

    int nprobes = atoi(argv[2]);
    // printf("Enter the number of probes: ");
    // scanf("%d", &nprobes);

    int timediff = atoi(argv[3]);
    // printf("Enter the time difference in miliseconds: ");
    // scanf("%d", &timediff);

    // if the input is a site name, convert it to IP address
    struct hostent *host = gethostbyname(site);
    if(host == NULL){
        printf("Error in gethostbyname");
        exit(1);
    }

    char* ip = inet_ntoa(*((struct in_addr *)(host->h_addr_list[0])));
    if (ip == NULL){
        printf("Error in inet_ntoa");
        exit(1);
    }
    printf("Routing to IP address: %s\n", ip);
    
    // create a raw socket
    int sock_fd;
    if((sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
    {
        perror("socket() error: ");
        exit(-1);
    }

    // set the timeout for the socket
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        perror("Setsockopt error: ");
        exit(1);
    }

    // create the sockaddr_in structure for the website
    struct sockaddr_in website_address;
    website_address.sin_family = AF_INET; 
    website_address.sin_addr.s_addr = inet_addr(ip);

    char route[100][100];
    double latency[100];
    double bandwidth[100];

    int ttl = 1;
    
    //create an icmp packet of 0 data
    char* packet = (char*)malloc(64 * sizeof(char));
    memset(packet, 0, 64);
    struct icmp *icmp = (struct icmp*)packet;
    icmp->icmp_type = ICMP_ECHO;
    icmp->icmp_code = 0;
    icmp->icmp_cksum = 0;
    icmp->icmp_id = (getpid() & 0xFFFF);
    icmp->icmp_seq = 0;
    icmp->icmp_cksum = checksum((unsigned short *) icmp, 64 + sizeof(struct icmphdr));
    int dest_reach = 0;
    int complete_route = 1;
    while(1){
        // set the ttl
        if(setsockopt(sock_fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0){
            perror("Setsockopt error: ");
            exit(1);
        }

        // before finalizing the intermediate hop, we should send atleast 5 icmp packets with proper headers each 1 second apart, per intermediate hop
        // so we make keep on sending packets until we get the same ip from 5 consecutive packets
        int count = 0;
        char* prev_ip = (char*)malloc(100 * sizeof(char));
        char* curr_ip = (char*)malloc(100 * sizeof(char));
        struct sockaddr_in inter_addr;
        struct pollfd pfd;
        pfd.fd = sock_fd;
        pfd.events = POLLIN;
        int tries = 0;
        while(count < 5){
            // send the packet
            printf("Sending packet with ttl = %d\n", ttl);
            int send_status;
            if((send_status = sendto(sock_fd, packet, 64, 0, (struct sockaddr*)&website_address, sizeof(website_address))) < 0){
                perror("Sendto error: ");
                exit(1);
            }
            // printf("Packet sent: %s and send_status =%d\n", packet, send_status);
            // receive the packet
            char* buffer = (char*)malloc(64 * sizeof(char));
            memset(buffer, 0, 64);
            // clear inter_addr
            memset(&inter_addr, 0, sizeof(inter_addr));
            socklen_t inter_addr_len = sizeof(inter_addr);

            // poll the socket for 1 second
            int ret = poll(&pfd, 1, 1000);
            if (ret < 0){
                perror("Poll error: ");
                exit(1);
            }
            else if (ret == 0){
                printf("Hop %d timed out at try %d\n", ttl, tries);
                if (tries == TRIES){
                    printf("No response from the next node in the route. Tried 3 times. Exiting\n\n");
                    dest_reach = 1;
                    complete_route = 0;
                    break;
                }
                tries++;
                continue;
            }

            if (dest_reach == 1) break;
            int recv_status;
            if((recv_status = recvfrom(sock_fd, buffer, 64, 0, (struct sockaddr*)&inter_addr, &inter_addr_len)) < 0){
                perror("Recvfrom error: ");
                exit(1);
            }
            // printf("Packet received: %s and recv_status = %d\n", buffer, recv_status);
            struct ip *ip_hdr = (struct ip*)buffer;
            struct icmp *icmp_hdr = (struct icmp*)(buffer + (ip_hdr->ip_hl << 2));
            // if the icmp header is of type ICMP_ECHOREPLY or ICMP_TIME_EXCEEDED, we ignore it
            if(icmp_hdr->icmp_type == ICMP_ECHOREPLY || icmp_hdr->icmp_type == ICMP_TIME_EXCEEDED){
                // printf("%d\n", icmp_hdr->icmp_type);
                if (icmp_hdr->icmp_type == ICMP_ECHOREPLY && count == 4){
                    // if we get the icmp_echoreply from the final destination, we break out of the loop
                    dest_reach = 1;
                }
            }
            else if (icmp_hdr->icmp_type == ICMP_DEST_UNREACH){
                perror("Destination Unreachable: ");
                exit(1);
            }
            else{
                printf("Unknown error\n");
                printf("Type: %d\n", icmp_hdr->icmp_type);
                printf("Code: %d\n", icmp_hdr->icmp_code);
                exit(1);
            }
            // get the ip address of the intermediate hop
            // printf("IP address recd: %s\n", inet_ntoa(inter_addr.sin_addr));
            // printf("gethostbyaddr: %s\n", gethostbyaddr((char*)&inter_addr.sin_addr, sizeof(inter_addr.sin_addr), AF_INET)->h_name);
            strcpy(curr_ip, inet_ntoa(inter_addr.sin_addr));
            free(buffer);
            // if the current ip is same as the previous ip, we increment the count
            if(strcmp(curr_ip, prev_ip) == 0){
                count++;
                strcpy(prev_ip, curr_ip);
            }
            else{
                count = 1;
                strcpy(prev_ip, curr_ip);
            }
            sleep(1);
        }

        if (dest_reach == 1 && complete_route == 0) break;
        // now the next hop is finalised
        // we store the ip address and the latency in the arrays
        if (!dest_reach) printf("\n--------------------Intermediate hop %d: %s------------------------\n", ttl, curr_ip);
        else printf("\n--------------------Destination: %s------------------------\n", curr_ip);
        strcpy(route[ttl], curr_ip);
        // now we call the function which will return the latency and bandwidth
        latency_bandwidth_function(curr_ip,nprobes,timediff,sock_fd,ttl,bandwidth, latency);
        free(prev_ip);
        free(curr_ip);
        if (dest_reach == 1) break;
        ttl++;
    }
    close(sock_fd);
    // print the route
    if (!complete_route) printf("The route to the website is incomplete. So showing the route till the last reached node\n");
    printf("The route to the website is:\n");
    // print the route in well formatted order
    for(int i = 1; i <= ttl; i++){
        if (!complete_route && i == ttl) continue;
        printf("%d. %s\n", i, route[i]);
    }
    free(packet);
    return 0;
}
