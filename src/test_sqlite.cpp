#include "ef_db.h"
#include "ef_file.h"
#include <stdio.h>
#include <pthread.h>
#include <iostream>
#include <atomic>
#include <time.h>

std::atomic<int>    out;
//pthread_cond_t      condition=PTHREAD_COND_INITIALIZER;

const ef_db *ef_db::_instance=new ef_db();
file_info_buf buf;

void *do1(void *arg){
    std::string str("/");
    ef_file f(buf);
    out=1;
    f._traverse_file_system(str); 
    out=0;   
}

void *do2(void *arg){
    ef_db           *db_instance=ef_db::get_instance();
    file_info        info;
    std::string      insert_info;
    int              insert_size;
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
    ef_db::destory_instance();
}

int main(){
    clock_t _begin,_end;
    _begin=clock();
    void *status;
    std::cout<<"begin..."<<std::endl;
    pthread_t pid;
    out=0;
    pthread_create(&pid,NULL,do1,0);
    sleep(5);
    pthread_create(&pid,NULL,do2,0);
    pthread_join(pid,&status);
    _end=clock();
    std::cout<<"time used:"<<(double)(_end-_begin)/CLOCKS_PER_SEC<<std::endl;
    return 0;
}
