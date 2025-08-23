#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
char* filename(char* path){
    char* p=path+strlen(path);
    while(p>=path && *p!='/') --p;
    return ++p;
}
void find(char* source,char* target){
    char path[512];
    int fd;
    struct stat st;
    struct dirent de;

    if((fd=open(source,O_RDONLY))<0){
        printf("Open path failed:%s\n",source);
        exit(0);
    }    
    if(fstat(fd,&st)<0){
        printf("Obtain the file info failed,fd:%d\n",fd);
        close(fd);
        exit(0);
    }
    switch(st.type){
        case T_DEVICE:
            break;
        case T_FILE:
            if(strcmp(filename(source),target)==0)
                printf("%s\n",source);
            break;
        case T_DIR:
            if(strlen(source)+1+DIRSIZ+1>sizeof(path)){
                printf("find:path too long:%s\n",source);
                break;
            }
            strcpy(path,source);
            char*p=path+strlen(source);
            *p++='/';
            while(read(fd,&de,sizeof(de))==sizeof(de)){
                if(de.inum==0||strcmp(".",de.name)==0||strcmp("..",de.name)==0) continue;
                strcpy(p,de.name);
                find(path,target);  
            }
            break;
    }
    close(fd);
}
int main(int argc,char* argv[]){
    if(argc<3) {
        printf("Find [path] [target]\n");
        exit(0);
    }
    find(argv[1],argv[2]);
    exit(0);
}