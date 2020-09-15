/*
#include "ef.h"

void *fileio_in_buf(void *arg){
    std::string str("/");
    ef_db *buf=(ef_db*)arg;
    ef_file f(buf);
    
    f._traverse_file_system(str);
}

void *db_out_buf(void *arg){
    ef_db       *db_instance=ef_db::get_instance();
    file_info    info;
    std::string  insert_info;
    int          insert_size;
    insert_info.resize(4096);
    while(1){
        if(buf.dequeue(info)==false){
            if(out==0){
                break;
            }
            continue;
        }
        sprintf(&insert_info[0],"INSERT INTO files values(\
\"%s\",\"%d\",datetime(%ld,\"unixepoch\"))\0",info.file_name.data(),info.file_type,info.file_time);
        for(insert_size=0;insert_size<4096;insert_size++) if(insert_info[insert_size]=='\0') break;
        insert_info.resize(insert_size);
        std::cout<<insert_info<<std::endl;
        db_instance->get_db().execute(insert_info.data());
        insert_info.resize(4096);
    }
}

void *db_in_buf(void *arg){
    ef_db           *db_instance=ef_db::get_instance();
    file_info        info;
    std::string      insert_info;
    int              insert_size;
    insert_info.resize(4096);
    sprintf()
}

void *io_out_buf(void *arg){

}

ef::ef(){
    db_instance=ef_db::get_instance();
    file_instance=new ef_file(ef_buf);
}

ef::~ef(){
    db_instance->destory_instance();
    db_instance=NULL;
    delete file_instance;
    file_instance=NULL;
}

void ef::reset(){
    pthread_t   pid;
    void       *status;
    pthread_create(&pid,NULL,fileio_in_buf,0);
    pthread_create(&pid,NULL,db_out_buf,0);
    pthread_join(pid,&status);
}

void ef::search(){

}

*/