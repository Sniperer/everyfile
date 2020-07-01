#ifndef _LOCK_FILE_H
#define _LOCK_FILE_H

#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>

int lock_file(int fd,int cmd,int type,off_t offset,int whence,off_t len){
    struct flock lock;

    lock.l_type=type;
    lock.l_start=offset;
    lock.l_whence=whence;
    lock.l_len=len;

    return (fcntl(fd,cmd,&lock));
}

#define write_lock_whole_file(fd) \
    lock_file((fd),F_SETLK,F_WRLCK,(0),(SEEK_SET),(0))

#define write_unlock_whole_file(fd) \
    lock_file((fd),F_SETLKW,F_WRLCK,(0),(SEEK_SET),(0))

#endif