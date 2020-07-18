#ifndef _EF_DB_H
#define _EF_DB_H

#include "db/sqlite3ppext.h"
#include <pthread.h>
using namespace sqlite3pp;

/*
 *  Thread-safety singleton design to make sure having only one instance. 
 */

class ef_db{
    public:
        static ef_db           *get_instance();
        static void             destory_instance();
        database               &get_db();
    private:
        ef_db();
        static const ef_db     *_instance;
        static pthread_mutex_t  destory_mutex;
        database                db;
};

#endif