#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>

#include "buf.h"
#include "sdl.h"

#define BUFLEN 65535 //Max length of buffer
#define PORT 2333

int total_len = 0;
char local_buf[BUFLEN];

void die(char * s) {
    perror(s);
    exit(1);
}

void copy_to_buffer(char *data, int len){

    if(save_data(data, len) != len){
        printf("buffer overflow....\n");
    }
    printf("saving done. cur size %d\n", get_data_cnt());
    return;
}

void *recv_video_data(void *thread_id){
    struct sockaddr_in si_me, si_other;
    long tid;
    int s, slen = sizeof(si_other), recv_len;

    tid = (long)thread_id;
    printf("recving thread id %ld is running\n", tid);
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        pthread_exit(NULL);
        return NULL;
    }

    memset((char * ) & si_me, 0, sizeof(si_me));

    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    //bind socket to port
    if (bind(s, (struct sockaddr * ) & si_me, sizeof(si_me)) == -1) {
        pthread_exit(NULL);
        close(s);
        return NULL;
    }

    //keep listening for data
    while (1) {
        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(s, local_buf, BUFLEN, 0, (struct sockaddr * ) &si_other, (socklen_t *)&slen)) == -1) {
            break;
        }

        total_len += recv_len;
        //printf("copying data len %d\n", total_len);
        copy_to_buffer(local_buf, recv_len);
    }

    close(s);
    pthread_exit(NULL);
    return NULL;
}

int main(void) {
    long tid = 1000;
    int ret;
    init_buffer();

    pthread_t threads[5];
    ret = pthread_create(&threads[0], NULL, recv_video_data, (void *)tid);

    if(ret){
        printf("Error: recving thread creation failed.\n");
        return -1;
    }

    while(1){
        sleep(1);
        if(get_data_cnt() > 3000000) break;
        printf("waiting.....\n");
    }

    sdl_main();
    pthread_exit(NULL);
    printf("done\n");

    return 0;
}
