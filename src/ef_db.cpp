#include "ef_db.h"

pthread_mutex_t ef_db::destory_mutex=PTHREAD_MUTEX_INITIALIZER;

ef_db::ef_db(){
    db=std::move(sqlite3pp::database("../ef.db"));
    db.execute("CREATE TABLE IF NOT EXISTS `files`( \
        `file_name` varchar(255) PRIMARY KEY NOT NULL, \
        `file_type` varchar(20) NOT NULL,\
        `file_time` DATATIME)");
}

ef_db* ef_db::get_instance(){
    return const_cast<ef_db*>(_instance);
}

void ef_db::destory_instance(){
    pthread_mutex_lock(&destory_mutex);
    if(_instance!=NULL){
        delete _instance;
        _instance=NULL;
    }
    pthread_mutex_trylock(&destory_mutex);
}

sqlite3pp::database& ef_db::get_db(){
    return db;
}