#include "user/user.h"
#include "kernel/types.h"
#include "kernel/stat.h"

int main(int argc,char *argv[]){
    int p[2];
    pipe(p);
    char buffer=0;
    int flag=0;
    int id=fork();
    if(id<0){
        printf("fork error");
        exit(1);
    }
    else if(id==0){
        if((flag=read(p[0],&buffer,1))!=1){
            printf("child read error");
        }
        close(p[0]);
        if((flag=write(p[1],&buffer,1))!=1){
            printf("child write error");
        }
        close(p[1]);
        if(flag==1) printf("%d: received ping\n",getpid());
    }
    else{
        if((flag=write(p[1],&buffer,1))!=1){
            printf("parent read error");
        }
        close(p[1]);
        wait((int*)0);
        if((flag=read(p[0],&buffer,1))!=1){
            printf("parent read error");
        }
        close(p[0]);
        if(flag==1) printf("%d: received pong\n",getpid());
    }
    exit(0);
}