#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

int main(int argc,char* argv[]){
    if(argc<2) {
        printf("Arguments < 2");
        exit(0);
    }
    char buffer[512],*p=buffer;
    while(read(0,p,1)==1){
        while(*p++!='\n'&&read(0,p,1)==1) ;
        *(p-1)=0;
        if(fork()==0){
            char *av[MAXARG];
            av[0]=argv[1];
            int p1=1;
            int ac=argc;
            for(int i=2;i<argc;++i)
                av[p1++]=argv[i];
            av[ac-1]=buffer;
            av[ac]=0;
            exec(av[0],av);
        }   
        else{
            p=buffer;
            wait((int*)0);
            continue;
        }
    }
    exit(0);
}



