#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>


#define BUFFER_SIZE  5*1024*1024
//#define BUFFER_SIZE  1024*1024
static char *buf;
static int head = 0;
static int tail = 0;

char *init_buffer(){
    buf = malloc(BUFFER_SIZE);
    return buf;
}


char *get_buf_ptr(){
    return buf;
}

int get_buf_size(){
    return BUFFER_SIZE;
}


int save_data(char *data, int len){
    int e;

    if(head < tail){
        e = tail - head;
        if(e < len){
            printf("avail space %d. less than input len %d\n", e, len);
            return -1;
        }
        memcpy(buf+head, data, len);
        head += len;
    }
    else{
        int rleft = BUFFER_SIZE - head;
        if(rleft >= len){
            memcpy(buf+head, data, len);
            head += len;
        }
        else{
            e = rleft + tail;
            if(e < len){
                printf("avail space %d. less than input len %d\n", e, len);
                return -1;
            }
            memcpy(buf+tail, data, rleft);
            memcpy(buf, data+rleft, len-rleft);
            head = len - rleft;
        }
    }
    return len;
}

int get_data(char *p, int len){
    int c;
    if(head == tail) return 0;

    if(head > tail){
        c = head - tail;
        if (c > len){
            memcpy(p, buf+tail, len);
            tail += len;
            return len;
        }else{
            memcpy(p, buf+tail, c);
            tail += c;
            return c;
        }
    }
    else{
        int rcnt = BUFFER_SIZE - tail;
        if(rcnt > len){
            //just read
            memcpy(p, buf+tail, len);
            tail += len;
            return len;
        }else{
            int left;
            memcpy(p, buf+tail, rcnt);
            left = len - rcnt;
            if(head >= left){
                memcpy(p+rcnt, buf, left);
                tail = left;
                return len;
            }else{
                memcpy(p+rcnt, buf, head);
                tail = head;
                return rcnt+head;
            }
        }
    }
}

int get_data_cnt(){
    int c;
    if(head == tail) return 0;

    if(head > tail){
        c = head - tail;
        return c;
    }
    else{
        int rcnt = BUFFER_SIZE - tail;
        return rcnt+head;
    }
}

int cmp_array(char *s1, char *s2, int len){
    int i;
    for(i=0; i < len; i++){
        if(*s1++ != *s2++) return i;
    }

    return 0;
}
