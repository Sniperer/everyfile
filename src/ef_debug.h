#ifndef _EF_DEBUG_H
#define _EF_DEBUG_H

#include <string>
#include <iostream>

template<typename T,typename... TAIL>
void debug_info(T t,TAIL... tail);

#ifdef DEBUG
    template<typename T,typename... TAIL>
    void debug_info(T t,TAIL... tail){
        std::string buf(4096,'\0');
        sprintf(&buf[0],t,tail...);
        std::cerr<<buf<<std::endl;        
    }
#else
    template<typename T,typename... TAIL>
    void debug_info(T t,TAIL... tail){
        
    }
#endif

#endif