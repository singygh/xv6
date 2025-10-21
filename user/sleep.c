#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int main(int argc,char *argv[])
{
    if(argc <= 1){
        fprintf(2, "usage: you need one arg\n");
        exit(0);
    }
    char* timestr = argv[1];
    int time = atoi(timestr);
    pause(time);
    exit(0); 
}