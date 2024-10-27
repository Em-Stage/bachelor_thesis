#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>

struct thread_arg {
    struct sockaddr_in serv_addr;
    uint8_t *dgram, *flag;
    uint32_t mes_size, sockfd, framerate, return_frames, return_bytes;
    double time_rate, time_elapsed, flag1;
    double* flag_time;
    double* time_elap; 
    socklen_t socksize;
} thread_arg;

uint32_t sock_create(uint32_t sockfd) {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
    if (sockfd < 0) {
        perror("Sock creation failed");
        exit(1);
    }
    return sockfd;
}

uint8_t* data_construction (uint32_t size, uint8_t* dgram) {
    for (int i = 0; i < (size / 4); ++i) {
        dgram[i] = rand(); 
    }
    return dgram;
}

uint32_t done(uint8_t* message) {
    uint8_t check [] = {100, 111, 110, 101};
    for (int i = 0; i < 3; i++) {
        if (message[i] != check[i]) {
            return 1; 
        }
    }
    return 0;
}

uint8_t* create_flag(uint8_t* dgram) {
    uint8_t flag [] = {100, 101, 102, 103, 104, 105, 106, 107, 108, 109}; //could be random instead, here hardcoded
    for (int i = 0; i < 10; ++i) {
        dgram[i] = flag[i]; 
    }
    return dgram;
}

uint8_t* add_flag(uint32_t size, uint8_t* dgram, uint8_t* flag) {
    for (int i = 0; i < 10; ++i) {
        dgram[i] = flag[i]; 
    }
    for (int i = 10; i < (size / 4); ++i) {
        dgram[i] = rand(); 
    }
    return dgram;
}

uint32_t flag_checker(uint8_t* message, uint8_t* flag) {
    for (int i = 0; i < 10; i++) {
        if (message[i] != flag[i]) {
            return 1; 
        }
    }
    return 0;
}

double* add_elipsed_time(double* elip, double time) {
    elip[0] = time; 
    return elip; 
}

double check_time(double *elip) {
    return elip[0]; 
}

void *data_sending (void *arg) {
    struct thread_arg sending_arg = *(struct thread_arg*) arg;
    uint32_t framerate = sending_arg.framerate; 
    uint32_t init_framerate = framerate; 
    double end = sending_arg.time_rate; 
    sending_arg.time_elapsed = 0.0;
    struct timespec start, fin; 
    uint32_t flag = 0;
    double time_between = (end / (double) init_framerate);
    
    //start timer - only sending thread holds time
    clock_gettime(CLOCK_REALTIME, &start ); 

    while(1) {
        clock_gettime(CLOCK_REALTIME, &fin); 
        sending_arg.time_elap = add_elipsed_time(sending_arg.time_elap, sending_arg.time_elapsed = ((double) (fin.tv_sec - start.tv_sec) + 1.0e-9 * (fin.tv_nsec - start.tv_nsec)));

        //break condition
        if ((framerate == -1) || (sending_arg.time_elapsed > end)) {
            sendto(sending_arg.sockfd, "done", 4, 0, (struct sockaddr *)&sending_arg.serv_addr, sending_arg.socksize);
            break; 
        }

        double between = sending_arg.time_elapsed - time_between;
        if (between > 0.0) {
            time_between += (end / (double) init_framerate);
            if (sending_arg.time_elapsed > 60 && flag == 0) {
                sending_arg.flag_time = add_elipsed_time(sending_arg.time_elap, sending_arg.time_elapsed = ((double) (fin.tv_sec - start.tv_sec) + 1.0e-9 * (fin.tv_nsec - start.tv_nsec)));
                double flag2 = check_time(sending_arg.flag_time);
                printf("Timestamp for sending flag: %f \n", flag2); 
                flag = 1; 
                sending_arg.dgram = add_flag(sending_arg.mes_size, sending_arg.dgram, sending_arg.flag); 
                sendto(sending_arg.sockfd, sending_arg.dgram, sending_arg.mes_size, 0, (struct sockaddr *)&sending_arg.serv_addr, sending_arg.socksize);
                bzero(sending_arg.dgram, sending_arg.mes_size); 
                framerate -= 1;
                continue;
            }

            //construct dgram then send 
            sending_arg.dgram = data_construction(sending_arg.mes_size, sending_arg.dgram); 
            sendto(sending_arg.sockfd, sending_arg.dgram, sending_arg.mes_size, 0, (struct sockaddr *)&sending_arg.serv_addr, sending_arg.socksize);
            bzero(sending_arg.dgram, sending_arg.mes_size); 

            //add total_dgrams in bytes and reduce framerate
            framerate -= 1; 
        }     
    }
    printf("Time elapsed: %f\n", sending_arg.time_elapsed);  
    pthread_exit((void *) (size_t) init_framerate - framerate); 
}

void *data_receive (void *arg) {
    struct thread_arg recv_arg = *(struct thread_arg*) arg;
    uint32_t return_bytes;
    uint32_t return_frames = 0; 
    double flag2, time_elapsed, latency; 

    while(1) {
        // break condition
        if (done (recv_arg.dgram) == 0) { break; } 

        if (flag_checker (recv_arg.dgram, recv_arg.flag) == 0) {
            time_elapsed = check_time(recv_arg.time_elap);  
            recv_arg.flag_time = add_elipsed_time(recv_arg.time_elap, check_time(recv_arg.time_elap));
            flag2 = check_time(recv_arg.flag_time); 
            printf("Timestamp for receiving flag: %f \n", flag2); 
        }

        //receive the returned datagram 
        bzero(recv_arg.dgram, recv_arg.mes_size); 
        return_bytes = recvfrom(recv_arg.sockfd, recv_arg.dgram, recv_arg.mes_size, 0, (struct sockaddr *)&recv_arg.serv_addr, &recv_arg.socksize);
        return_frames += (return_bytes / recv_arg.mes_size);
    }
    pthread_exit((void *) (size_t) return_frames -1); //adjust for datagram with break condition
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        perror("Number of arguments invalid. Format: ip-adress, framesize, framerate, timer, optional 1"); 
        exit(1); 
    }

    //setup
    uint32_t sockfd, sin_size, framerate, framecopy, mes_size, return_bytes, return_frames;
    struct sockaddr_in serv_addr;
    struct hostent *host;
    void *tot_frames;
    void *send_frames; 
    double *time_elap = (double*) malloc(sizeof(double) * 4);
    uint8_t *dgram_send = (uint8_t*) malloc(sizeof(uint8_t) * 4046); //allocate byte array
    uint8_t *dgram_recv = (uint8_t*) malloc(sizeof(uint8_t) * 4046); //allocate byte array
    uint8_t *flag = (uint8_t*) malloc(sizeof(uint8_t) * 10);
    double time_rate, time_elapsed;
    pthread_t pid_1, pid_2;
    socklen_t socksize = sizeof(struct sockaddr);

    //setup using argv 
    host = (struct hostent *) gethostbyname((char *) argv[1]);
    mes_size = atoi(argv[2]); 
    framerate = atoi(argv[3]); 
    time_rate = atoi(argv[4]); 
    framecopy = framerate;

    //create flag
    flag = create_flag(flag); 
    
    //create socket
    sockfd = sock_create(sockfd);

    //bind sockets
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000);
    serv_addr.sin_addr = *((struct in_addr *)host->h_addr);

    //create struct for sending thread 
    struct thread_arg sendargs;
    sendargs.dgram = dgram_send; 
    sendargs.mes_size = mes_size;
    sendargs.sockfd = sockfd; 
    sendargs.serv_addr = serv_addr;
    sendargs.socksize = socksize; 
    sendargs.framerate = framerate; 
    sendargs.time_rate = time_rate; 
    sendargs.flag = flag;
    sendargs.time_elapsed = time_elapsed;
    sendargs.time_elap = time_elap;

    //create struct for receiving thread
    struct thread_arg recvargs; 
    recvargs.dgram = dgram_recv; 
    recvargs.mes_size = mes_size; 
    recvargs.sockfd = sockfd; 
    recvargs.serv_addr = serv_addr; 
    recvargs.socksize = socksize; 
    recvargs.return_frames = return_frames;
    recvargs.return_bytes = return_bytes;
    recvargs.flag = flag; 
    recvargs.time_elap = time_elap; 
    
    pthread_create(&pid_2, NULL, &data_receive, &recvargs);
    pthread_create(&pid_1, NULL, &data_sending, &sendargs);

    pthread_join(pid_1, &send_frames); 
    pthread_join(pid_2, &tot_frames); 

    //convert and print total bytes received / send, to check for datagram loss
    printf("Datagram packets were of size: %d bytes\n", mes_size);
    printf("The testing equipment sent out %zu datagrams \n", (size_t)(off_t) send_frames); 
    printf("The DUT returned: %zu datagrams \n", (size_t)(off_t) tot_frames);
    printf("Number of datagrams lost is: %zu \n", (size_t)(off_t) send_frames - (size_t)(off_t) tot_frames); 

    //close socket
    close(sockfd);

    //free dgram
    free (dgram_recv); 
    free (time_elap); 
    free (flag); 
    free (dgram_send); 

    return 0;
}