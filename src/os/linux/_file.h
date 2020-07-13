#ifndef _LINUX_FILE_H
#define _LINUX_FILE_H

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>


class _file{
    public:
        enum            _file_type
        {LNK,REG,DIR,CHR,BLK,FIFO,SOCK,ERR};
        int             _traverse_file_system(const char *_root);
        _file_type      _get_file_type(mode_t &_m);
    private:
        virtual int     add_file_info(std::string &&_msg)=1;
}

#endif