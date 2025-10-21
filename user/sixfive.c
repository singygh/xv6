#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int main(int argc,char *argv[])
{

    if (argc<=1)
    {
        fprintf(2, "usage: you need one arg\n");
        exit(0);
    }
    char* template = " -\r\t\n./";
    char* template1 = "0123456789";
    char* nums[100];
    int index0 = 0;
    char temp[10];
    int index = 0;
    char a[1];

    for (int i = 1;i<argc;i++)
    {
        char* filename = argv[i];
        int fd = open(filename,O_RDONLY);
        for(;;)
        {
            int n = read(fd,a,1);
            if (n==0) break;
            char *c = strchr(template,*a);
            if (c == 0)
            {
                char* d = strchr(template1,*a);
                if(d!=0)
                {
                    temp[index] = *a;
                    index++;
                    continue;
                }
            }
            else
            {
                if (index != 0)
                {
                    temp[index] = '\0';
                    index = 0;
                    nums[index0] = malloc(strlen(temp) + 1);
                    memcpy(nums[index0],temp,10);
                    printf("hello");
                    index0++;
                    memset(temp,0,10);
                }
            }
        }
        if (index!=0) 
        {
            temp[index] = '\0';
            nums[index0] = malloc(strlen(temp) + 1);
            memcpy(nums[index0],temp,10);
            index0++;
        }
        close(fd);
    }

    for(int i = 0;i<index0;i++)
    {
        // printf("%s\n",nums[i]);
        int num = atoi(nums[i]);
        if ((num % 5 == 0) || (num % 6 == 0))
        {
            printf("%d\n",num);
        }
        free(nums[i]);
    }
    exit(0);
}