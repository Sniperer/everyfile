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
    file_info info;
    
}

int main(){
    pthread_t pid;
    pthread_create(&pid,NULL,do1,0);
    pthread_create(&pid,NULL,do2,0);
    return 0;
}
