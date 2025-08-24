#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/riscv.h"

int
main(int argc, char *argv[])
{
  // your code here.  you should write the secret to fd 2 using write
  // (e.g., write(2, secret, 8)
  uint64 sz=(uint64)sbrk(PGSIZE*24);
printf("old = %p\n", (void*)sz);
  sz%=4096;
  printf("new = %p\n", (void*)sz);
  char buffer[8];
  if(sz<=4056){
    if(sz==0) strcpy(buffer,(const char*)(sz+20*PGSIZE+32));
    else strcpy(buffer,(const char*)(sz+23*PGSIZE+32));
  }
  else if(sz>4056&&sz<4064){
    int b23=sz-4056;
    int b24=8-b23;
    printf("hello,im 2");
    memmove(buffer,(const char*)(sz+23*PGSIZE-b24),b24);
    memmove(buffer+b24,(const char*)(sz+22*PGSIZE),b23);
  }
  else{
    strcpy(buffer,(const char*)(sz+22*PGSIZE+sz-4064));
  }
  write(2,buffer,8);
  exit(0);

}
