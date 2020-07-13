#ifndef _EF_FILE_UPDATE_H
#define _EF_FILE_UPDATE_H

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <iostream>
#include <string>
#include <cstring>

#include "datastruct/__no_lock_queue_with_CAS.h"
#include "../ef_err.h"

#ifdef linux
    #include "os/linux/_file.h"
#endif

class ef_file:public _file{
    typedef __no_lock_queue_with_CAS _file_info_buf;
    public:
        ef_file(_file_info_buf &_buf);
        ~ef_file();
        int                 add_file_info();
    private:
        _file_info_buf      &buf;
};

#endif