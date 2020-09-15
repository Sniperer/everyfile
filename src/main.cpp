/*
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "ef_db.h"
#include "ef_file.h"
#include "ef.h"
#include <pthread.h>
#include <iostream>
#include <atomic>
#include <time.h>

const ef_db         *ef_db::_instance=new ef_db();
file_info_buf        buf;

int main(int argc,char *argv[]){
    int c;
    int digit_optind=0;

    while(1){
        int this_option_optind=optind?optind:1;
        int option_index=0;
        static struct option long_options[]={
            {"reset",no_argument,0,'r'},
            {"search",required_argument,0,'s'},
            {""},
            {0,0,0,0}
        };
        c=getopt_long(argc,argv,"rs:",long_options,&option_index);
        if(c==-1)
            break;
        switch(c){
            case 0:
                printf("option %s",long_options[option_index].name);
                if(optarg)
                    printf("with arg %s",optarg);
                printf("\n");
                break;
            case 'r':
                
                break;
            case 's':
                
                break;
            case '?':
                break;
            default:
                printf("?? getopt returned character code 0%o ??\n",c);
        }
    }
    if(optind<argc){
        printf("non-option ARGV-elements:");
        while(optind<argc)
            printf("%s ",argv[optind++]);
        printf("\n");
    }
    exit(EXIT_SUCCESS);
}
*/