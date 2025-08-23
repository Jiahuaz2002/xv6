#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc,char* argv[]){
    int p[2];
    pipe(p);
    if(fork()==0){
        int cur=0;
        int num=0;
        close(p[1]);
        while(read(p[0],&cur,4)!=0){
            printf("prime %d\n",cur);
            int p1[2];
            pipe(p1);
            if(fork()==0){
                close(p[0]);
                close(p1[1]);
                p[0]=dup(p1[0]);
                close(p1[0]);
                continue;
            }
            else{
                close(p1[0]);
                while(read(p[0],&num,4)!=0){
                    if(num%cur!=0)
                        write(p1[1],&num,4);
                }
                close(p[0]);
                close(p1[1]);
                wait((int*)0);
                exit(0);
            }
        }
        close(p[0]);
        exit(0);
    }
    else{
        close(p[0]);
        for(int i=2;i<=280;++i){
            if(write(p[1],&i,4)!=4) printf("parent write error!\n");
        }
        close(p[1]);
        wait((int*)0);
    }
    exit(0);
}

