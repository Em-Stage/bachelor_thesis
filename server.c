#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

uint32_t sock_create(uint32_t sockfd){
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
    if (sockfd < 0) {
        perror("Sock creation failed");
        exit(1);
    }
    return sockfd;
}

uint32_t bind_sock(uint32_t bind){
    if (bind < 0) {
        perror("Binding of socket failed");
        exit(1);
    }
    return bind;
}

uint32_t done(uint8_t* message) {
    uint8_t check [] = {100, 111, 110, 101};
    for (uint32_t i = 0; i < 3; i++) {
        if (message[i] != check[i]) {
            return 1; 
        }
    }
    return 0;
}

int main(int argc, char *argv[]){
    //setup 
    uint32_t sockfd, binding, bytes_read, total_bytes;
    uint8_t *dgram = (uint8_t*) malloc(sizeof(uint8_t) * 4046); //allocate byte array
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t socksize = sizeof(struct sockaddr);

    //setup sock
    sockfd = sock_create(sockfd);

    //setup bindings
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    //setup bind
    binding = bind(sockfd, (const struct sockaddr*) &serv_addr, sizeof(serv_addr));
    binding = bind_sock(binding);

    total_bytes = 0;

    while (1)
    {
        //first receive the datagram
        bytes_read = recvfrom(sockfd, dgram, 4045, 0, (struct sockaddr *)&serv_addr, &socksize);

        sendto(sockfd, dgram, bytes_read, 0, (struct sockaddr *)&serv_addr, socksize);

        // break condition
        if (done (dgram) == 0) { 
            sendto(sockfd, "done", 4, 0, (struct sockaddr *)&serv_addr, socksize);
            break; 
            } 
        
        total_bytes += bytes_read; 
        bzero(dgram, 4045); 
    }

    printf("total bytes read: %d\n", total_bytes); 

    //free dgrams
    free (dgram); 

    //close socket
    close(sockfd);
    return 0;
}