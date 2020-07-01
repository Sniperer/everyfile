#include <iostream>
#include <cstdio>
#include <errno.h>
#include <cstring>

using namespace std;

#define MAXLINE 4096

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
    cerr<<buf<<endl;
}

template<typename T,typename... TAIL>
void err_quit(T t,TAIL... tail){
    err_doit(0,0,"%s[%d]",__FILE__,__LINE__);
    err_doit(0,0,t,tail...);
    exit(1);
}
int main(){
    err_quit("%s[%d]:%dtest!!\n%d",__FILE__,__LINE__,1,2);
    return 0;
}