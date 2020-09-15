/*
#ifndef _EF_H
#define _EF_H

#include "ef_db.h"
#include "ef_file.h"
#include <stdio.h>
#include <pthread.h>
#include <iostream>
#include <atomic>
#include <time.h>
#include <string>

class ef{
    public:
        void               reset();
        void               search();
        ef();
        ~ef();
    private:
        ef_db             *db_instance;
        ef_file           *file_instance;
        file_info_buf      ef_buf;
};

#endif

*/