#include <cstdio>
#include <cstdlib>
#include <getopt.h>
#include "ef_db.h"
#include "ef_file.h"
#include <pthread.h>
#include <iostream>
#include <atomic>
#include <time.h>

const ef_db         *ef_db::_instance=new ef_db();
file_info_buf        buf;
std::atomic<int>     out;

void print_res(q_res *r){
	for(sqlite3pp::query::iterator iter=r->begin();iter!=r->end();++iter){
		std::string _a,_b,_c;
			(*iter).getter()>>_a>>_b>>_c;
		std::cout<<_a<<std::endl;	
	}
}

void *out_sys_files(void *){
	std::string str("/");
	ef_file f(buf);
	out=1;
	f._traverse_file_system(str);
	out=0;
	return NULL;
}

void *in_db(void *){
	ef_db		*db_instance=ef_db::get_instance();
	file_info	 info;
	while(1){
		if(buf.dequeue(info)==false){
			if(out==0){
				break;
			}
			continue;
		}
		db_instance->add(info);
	}
	ef_db::destory_instance();
	return NULL;
}

int main(int argc,char *argv[]){
    ef_db		*db_instance = ef_db::get_instance();
	std::string  qry;
	q_res       *res;
	pthread_t	 pid;
	void        *status;
	int c;
    int digit_optind=0;
    int option_index=0;
    static struct option long_options[]={
        {"exact",required_argument,0,'e'},
		{"fuzzy",required_argument,0,'f'},
        {"init",no_argument,0,'i'},
		{"help",no_argument,0,'h'},
        {0,0,0,0}
    };
    c=getopt_long(argc,argv,"e:f:ih",long_options,&option_index);
    switch(c){
        case 'e':
			qry=argv[optind-1];
			std::cout<<qry<<std::endl;
			res=db_instance->qry_exact(qry);
			//printf("%d\n",res==NULL);
			print_res(res);
			break;
        case 'f':
            //printf("%s\n", argv[optind-1]);
            qry=argv[optind-1];
			res=db_instance->qry_no_exact(qry);
			//printf("%d\n",res==NULL);
			print_res(res);
			break;
        case 'i':
			//printf("init.\n");
            out=0;
			pthread_create(&pid,NULL,out_sys_files,0);
			sleep(5);
			pthread_create(&pid,NULL,in_db,0);
			pthread_join(pid, &status);
			break;
		case 'h':
			//printf("help.\n");
			printf("--exact[-e] filename: search filename exactly.\n");
			printf("--fuzzy[-f] filename: search filename fuzzy.\n");
			printf("--init: init ef_db. This step would cost a long time.\n");
			break;
        default:
			printf("error option.\n");
    }
    exit(EXIT_SUCCESS);
}
