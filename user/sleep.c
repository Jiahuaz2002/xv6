#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc,char*argv[]){
    if(argc<2) {
        printf("Argument Missing!");
        exit(0);
    }
    int n=0;
    if((n=atoi(argv[1]))==0) exit(0);
    sleep(n);
    exit(0);

}