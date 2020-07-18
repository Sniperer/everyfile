#include "ef_db.h"
#include "ef_file.h"
#include <stdio.h>
#include <pthread.h>
#include <iostream>

const ef_db *ef_db::_instance=new ef_db();
file_info_buf buf;

void *do1(void *arg){
    std::string str("/");
    ef_file f(buf);
    f._traverse_file_system(str);    
}

void *do2(void *arg){
    ef_db           *db_instance=ef_db::get_instance();
    file_info        info;
    std::string      insert_info;
    int              insert_size;
    insert_info.resize(4096);
    while(1){
        if(buf.dequeue(info)==false)
            continue;
        sprintf(&insert_info[0],"INSERT INTO files values(\
\"%s\",\"%d\",datetime(%ld,\"unixepoch\"))\0",info.file_name.data(),info.file_type,info.file_time);
        for(insert_size=0;insert_size<4096;insert_size++) if(insert_info[insert_size]=='\0') break;
        insert_info.resize(insert_size);
        //std::cout<<insert_info<<std::endl;
        db_instance->get_db().execute(insert_info.data());
        insert_info.resize(4096);
    }
    ef_db::destory_instance();
}

int main(){
    std::cout<<"begin..."<<std::endl;
    pthread_t pid;
    pthread_create(&pid,NULL,do1,0);
    pthread_create(&pid,NULL,do2,0);
    while(1){
    }
    return 0;
}
