#ifndef _EF_ERR_H
#define _EF_ERR_H

#include <iostream>
#include <cstring>
#include <cstdio>
#include <errno.h>

#define MAXLINE 4096

template<typename T,typename... TAIL>
static void err_doit(int,int,T,TAIL...);

/**
 *  fatal error related to a system call. 
 */
template<typename T,typename... TAIL>
void err_sys(T t,TAIL... tail){
    err_doit(1,errno,t,tail...);
    exit(1);
}

/**
 * 
 */
template<typename T,typename... TAIL>
void err_exit(int error,T t,TAIL... tail){
    err_doit(1,error,t,tail...);
    exit(1);
}

/**
 *  unfatal error unrelated to a system call.
 */
template<typename T,typename... TAIL>
void err_msg(T t,TAIL... tail){
    err_doit(0,0,t,tail...); 
}

/**
 *  fatal error unrelated to a system call.
 */
template<typename T,typename... TAIL>
void err_quit(T t,TAIL... tail){
    err_doit(0,0,t,tail...);
    exit(1);
}

template<typename T,typename... TAIL>
static void err_doit(int errnoflag,int error,T t,TAIL... tail){
    std::string buf(MAXLINE,'\0');
    sprintf(&buf[0],t,tail...);
    for(int i=0;i<buf.size();i++){
        if(buf[i]=='\0')
            buf.resize(i);
        else
            continue;
    }
    if(errnoflag){
        std::string errinfo(MAXLINE,'\0');
        sprintf(&errinfo[0],": %s",strerror(error));
        buf+=errinfo;
    }
    std::cerr<<buf<<std::endl;
}

#endif