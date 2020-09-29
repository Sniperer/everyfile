#include <iostream>
#include <string>
#include <cstring>
#include <sys/time.h>
#include "ef_db.h"
#include "ef_file.h"

const ef_db *ef_db::_instance = new ef_db();
file_info_buf buf;

int main(){
	struct timeval tv;
	ef_db *db_instance = ef_db::get_instance();
	file_info info;
	info.file_name="/home/snipernie/test1";
	gettimeofday(&tv, NULL);
	info.file_time=tv.tv_sec;
	info.file_type=EREG;
	db_instance->add(info);
	return 0;
}
