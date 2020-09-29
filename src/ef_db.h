#ifndef _EF_DB_H
#define _EF_DB_H

#include "db/sqlite3pp.h"
#include "ef_file.h"
#include <pthread.h>
#include <map>
#include <vector>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/*
 *  Thread-safety singleton design to make sure having only one instance. 
 */

typedef sqlite3pp::query q_res;

class ef_db{
    public:
        static ef_db           *get_instance();
        static void             destory_instance();
        q_res				   *qry_exact(const std::string&);
		q_res				   *qry_no_exact(const std::string&);
		int						add(file_info);
		int						rm(const std::string&);
		int						mv(const std::string&, const std::string&);
		sqlite3pp::database    &get_db();
    private:
        ef_db();
        static const ef_db     *_instance;
        static pthread_mutex_t  destory_mutex;
        sqlite3pp::database     db;
};

#endif
