#include "ef_linux_file.h"

_file_info::_file_info(std::string &_name,_file_type &_type,time_t &_time):
    file_name(_name),file_type(_type),file_time(_time){

}

_file_info(const _file_info &_info){
    file_name=_info.file_name;
    file_type=_info.file_type;
    file_time=_info.file_time;
}

_file_info& _file_info::operator=(const _file_info &_info){
    file_name=_info.file_name;
    file_type=_info.file_type;
    file_time=_info.file_time;
}
/*
 *  Traverse sub-file-node in the file system by DFS.
 *  Whatever file type is, function would call the func_obj to solve
 *  it. If it's a drectory, call _traverse_file_system after 
 *  call the func_obj.
 *  Return 1 when occurs errors; return 0 when is successed to access;
 */
int _file::_traverse_file_system(std::string &_root){
    struct stat         statbuf;
    struct dirent      *dirp;
    _file_type          ftype;
    DIR                *dp;
    int                 ret,n;
    std::string         path;
    if(lstat(_root.data(),&statbuf)<0){
        err_msg("%s[%d]: can't call lstat",__FILE__,__LINE__);
        return 1;
    }
    ftype=_get_file_type(statbuf.st_mode);
    switch(ftypes){
        case LNK:
            break;
        case REG:
            _file_info finfo(_root,REG,statbuf.st_mtime)
            add_file_info(finfo);
            return 0;
        case DIR:
            std::string msg;
            add_file_info(std::move(msg));
            //traverse for all sub-file.
            if((dp=opendir(_root.data()))==NULL){
                err_msg("%s[%d]: can't open dir %s",__FILE__,__LINE__,_root.data());
                return 1;
            }
            while((dirp=readdir(dp))!=NULL){
                if(strcmp(dirp->d_name,".")==0 ||   \
                        strcmp(dir->d_name,"..")==0)
                    continue;
                path=_root+"/"+std::string(dirp->d_name);
                _traverse_file_system(path);
            }
            closedir(dp);
            return 0;
        case CHR:
            break;
        case BLK:
            break;
        case FIFO:
            break;
        case SOCK:
            break;
        default:
            err_msg("%s[%d]: unknown file type",__FILE__,__LINE__);
            return 1;
    }
}

_file_type _file::_get_file_type(mode_t &_m){
    swicth(_m&S_IFMT){
        case S_IFLNK:
            return LNK;
        case S_IFREG:
            return REG;
        case S_IFDIR:
            return DIR;
        case S_IFCHR:
            return CHR;
        case S_IFBLK:
            return BLK;
        case S_IFFIFO:
            return FIFO;
        case S_IFSOCK:
            return SOCK;
        default:
            return ERR;
    }
};