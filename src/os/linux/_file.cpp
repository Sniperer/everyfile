#include "ef_linux_file.h"

/*
 *  Traverse sub-file-node in the file system by DFS.
 *  Whatever file type is, function would call the func_obj to solve
 *  it. If it's a drectory, call _traverse_file_system after 
 *  call the func_obj.
 */
int _file::_traverse_file_system(const char *_root){
    struct stat         statbuf;
    struct dirent      *dirp;
    _file_type          ftype;
    DIR                *dp;
    int                 ret,n;
    if(lstat(_root,&statbuf)<0){
        err_msg("%s[%d]: can't call lstat");
        return 1;
    }
    ftype=_get_file_type(statbuf.st_mode);
    switch(ftypes){
        case LNK:
            break;
        case REG:
            std::string msg;
            add_file_info(std::move(msg));
            break;
        case DIR:
            std::string msg;
            add_file_info(std::move(msg));
        //traverse for all sub-file.
            _traverse_file_system();
            break;
        case CHR:
            break;
        case BLK:
            break;
        case FIFO:
            break;
        case SOCK:
            break;
        default:
            err_msg("%s[%d]: unknown file type");
            return 1;
    }
}
