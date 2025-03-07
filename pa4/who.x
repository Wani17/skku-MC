const MAXBUFFER=1000;

struct data {
    char result[MAXBUFFER];
    unsigned long size;
};

typedef struct data data;

program WHOPROG{
    version WHOVERS{
       data REMOTE_WHO(void) = 1;
    }=1;
}=0x12345678;
