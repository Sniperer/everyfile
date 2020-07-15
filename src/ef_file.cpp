#include "ef_file.h"

ef_file::ef_file(file_info_buf &_buf):buf(_buf){
    
}

ef_file::~ef_file(){
    
}

void ef_file::add_file_info(file_info &_info){
    //Is it neccessary to check buf.size()?
    buf.enqueue(_info);
}