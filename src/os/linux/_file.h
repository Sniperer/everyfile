#ifndef _LINUX_FILE_H
#define _LINUX_FILE_H

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <cstring>

#include "../../ef_err.h"
#include "../../ef_debug.h"

enum _file_type{
    ELNK,EREG,EDIR,ECHR,EBLK,EFIFO,ESOCK,ERR
};

class _file_info{
    public:
        _file_info()=default;
        ~_file_info()=default;

        _file_info(const _file_info &_info);
        _file_info& operator=(const _file_info &_info);
        int             init(std::string &_name,_file_type _type,__time_t &_time);
        
        std::string     file_name;
        _file_type      file_type;
        time_t          file_time;
};

class _file{
    public:
        int             _traverse_file_system(std::string &_root);
        _file_type      _get_file_type(mode_t &_m);
        virtual void    add_file_info(_file_info &_info)=0;
};

#endif