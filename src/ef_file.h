#ifndef _EF_FILE_UPDATE_H
#define _EF_FILE_UPDATE_H

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <iostream>
#include <string>
#include <cstring>

#include "datastruct/__no_lock_queue_with_CAS.h"
#include "ef_err.h"
#include "ef_debug.h"
#include "os/linux/_file.h"

typedef _file_info                              file_info;
typedef _file_type                              file_type;
typedef __no_lock_queue_with_CAS<file_info>     file_info_buf;

class ef_file:public _file{
    public:
        ef_file(file_info_buf &_buf);
        ~ef_file();
        void         add_file_info(file_info &_info);
    private:
        file_info_buf      &buf;
};

#endif
