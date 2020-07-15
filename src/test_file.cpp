#include "ef_file.h"
#include <iostream>
#include <string>
#include <pthread.h>

using namespace std;

file_info_buf buf;

void print(){
    file_info info;
    while(1){
        if(buf.dequeue(info)==false)
            continue;
        std::cout<<info.file_name<<"\n"<<info.file_type<<"\n"  \
            <<info.file_time<<"\n";
    }
}

void *doit(void * arg){
    string str("/");
    ef_file f(buf);
    f._traverse_file_system(str);    
}

int main(){
    pthread_t pid;
    pthread_create(&pid,NULL,doit,0);
    print();
    return 0;
}