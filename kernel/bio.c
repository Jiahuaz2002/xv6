// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"


#define HASH_SIZE 3
#define NBKT 3
#define WAYS 30


//static uint maxblockn=0;
struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  //struct buf head;
  struct buf* hash[NBKT][WAYS];
  struct spinlock idxlock[NBKT][WAYS];//lock per location
  struct spinlock bucketlock[NBKT];//bucket accesslock
  struct spinlock acquire_buf_lock[NBKT];//when find an empty lock in a bucket, acquire this lock
} bcache;

#define MULTIPLIER 2654435769U  // 这是 Knuth 建议的乘数
static inline int h(uint k) {
    return (unsigned int)(((uint64)k * MULTIPLIER) >> 28) % HASH_SIZE;
}
void
binit(void)
{
//  struct buf *b;

  initlock(&bcache.lock, "bcache");
  for(int i=0;i<NBKT;++i){
    for(int j=0;j<WAYS;++j){
      initlock(&bcache.idxlock[i][j],"bcache-idxlock");
    }
    initlock(&bcache.bucketlock[i],"bcache-bucketlock");
    initlock(&bcache.acquire_buf_lock[i],"bcache-acquirebuflock");
  }
  //struct buf* p=bcache.buf;
  //while(p!=bcache.buf+NBUF){

  //}
  /*for(int i=0;i<NBUF;++i){
    bcache.buf[i].valid=0;
    bcache.hash[i][0]=&bcache.buf[i];//NBUCK must bigger than NBUF
  }*/
 memset(bcache.hash, 0, sizeof(bcache.hash));
  struct buf* p=bcache.buf;
  for(int i=0;i<NBKT&&(p!=bcache.buf+NBUF);++i){
    for(int j=0;j<WAYS&&(p!=bcache.buf+NBUF);++j){
      p->valid=0;
      initsleeplock(&p->lock,"bufferlock");
      bcache.hash[i][j]=p++;

    }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b=0;
  int bkt=h(blockno);
  //first acquire the bucket lock
  acquire(&bcache.bucketlock[bkt]);
  //then for each element acquire the idx lock
  for(int i=0;i<WAYS;++i){
    acquire(&bcache.idxlock[bkt][i]);
    if(bcache.hash[bkt][i]&&bcache.hash[bkt][i]->blockno==blockno&&bcache.hash[bkt][i]->dev==dev){
       bcache.hash[bkt][i]->refcnt++;
       release(&bcache.idxlock[bkt][i]);
       release(&bcache.bucketlock[bkt]);
       acquiresleep(&bcache.hash[bkt][i]->lock);
       return bcache.hash[bkt][i];
    }
    release(&bcache.idxlock[bkt][i]);
  }
  //if not found then find a empty buf
  for(int i=0;i<NBKT;++i){
    acquire(&bcache.acquire_buf_lock[i]);
    for(int j=0;j<WAYS;++j){
      acquire(&bcache.idxlock[i][j]);
      if(bcache.hash[i][j]&&bcache.hash[i][j]->refcnt==0){
        b=bcache.hash[i][j];
        b->dev=dev;
        b->blockno=blockno;
        b->valid=0;
        b->refcnt=1;
        bcache.hash[i][j]=0;
        release(&bcache.idxlock[i][j]);
        break;
      }
      release(&bcache.idxlock[i][j]);
    }
    release(&bcache.acquire_buf_lock[i]);
    if(b) break;
  }
  if(!b) {
    release(&bcache.bucketlock[bkt]);
    panic("cant find an empty buf\n");
  }
  //in current bucket find an empty slot to insert the newly found buffer
  for(int i=0;i<WAYS;++i){
    acquire(&bcache.idxlock[bkt][i]);
    if(bcache.hash[bkt][i]==0){
      bcache.hash[bkt][i]=b;
      release(&bcache.idxlock[bkt][i]);
      release(&bcache.bucketlock[bkt]);
      acquiresleep(&b->lock);
      return b;
    }
    release(&bcache.idxlock[bkt][i]);
  }
  release(&bcache.bucketlock[bkt]);
  panic("no empty slot to store the buf\n");

}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int bkt=h(b->blockno);
  acquire(&bcache.bucketlock[bkt]);
  for(int i=0;i<WAYS;++i){
    acquire(&bcache.idxlock[bkt][i]);
    if(bcache.hash[bkt][i]==b){
        b->refcnt--;
        release(&bcache.idxlock[bkt][i]);
        break;
    }
    release(&bcache.idxlock[bkt][i]);
  }
  release(&bcache.bucketlock[bkt]);
  
}

void
bpin(struct buf *b) {

  int bkt=h(b->blockno);
  acquire(&bcache.bucketlock[bkt]);
  for(int i=0;i<WAYS;++i){
    acquire(&bcache.idxlock[bkt][i]);
    if(bcache.hash[bkt][i]==b){
        b->refcnt++;
        release(&bcache.idxlock[bkt][i]);
        break;
    }
    release(&bcache.idxlock[bkt][i]);
  }
  release(&bcache.bucketlock[bkt]);
}

void
bunpin(struct buf *b) {
  int bkt=h(b->blockno);
  acquire(&bcache.bucketlock[bkt]);
  for(int i=0;i<WAYS;++i){
    acquire(&bcache.idxlock[bkt][i]);
    if(bcache.hash[bkt][i]==b){
        b->refcnt--;
        release(&bcache.idxlock[bkt][i]);
        break;
    }
    release(&bcache.idxlock[bkt][i]);
  }
  release(&bcache.bucketlock[bkt]);
}


