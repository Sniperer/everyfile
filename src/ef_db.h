#ifndef _EF_DB_H
#define _EF_DB_H

#include "db/sqlite3pp.h"
#include <pthread.h>

/*
 *  Thread-safety singleton design to make sure having only one instance. 
 */

class ef_db{
    public:
        static ef_db           *get_instance();
        static void             destory_instance();
        sqlite3pp::database    &get_db();
    private:
        ef_db();
        static const ef_db     *_instance;
        static pthread_mutex_t  destory_mutex;
        sqlite3pp::database     db;
};

#endif