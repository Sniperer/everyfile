#include "ef_db.h"
#include <syslog.h>

pthread_mutex_t ef_db::destory_mutex=PTHREAD_MUTEX_INITIALIZER;

ef_db::ef_db(){
    db=std::move(sqlite3pp::database("/var/ef.db"));
    db.execute("CREATE TABLE IF NOT EXISTS `files`( \
        `file_name` varchar(255) PRIMARY KEY NOT NULL, \
        `file_type` varchar(20) NOT NULL,\
        `file_time` DATATIME)");
}

ef_db* ef_db::get_instance(){
    return const_cast<ef_db*>(_instance);
}

void ef_db::destory_instance(){
    pthread_mutex_lock(&destory_mutex);
    if(_instance!=NULL){
        delete _instance;
        _instance=NULL;
    }
    pthread_mutex_unlock(&destory_mutex);
}

sqlite3pp::database& ef_db::get_db(){
    return db;
}

q_res *ef_db::qry_exact(const std::string& _fname){
	std::string qry;
	int insert_size;
	qry.resize(4096);
	sprintf(&qry[0], "SELECT file_name, file_type, file_time FROM files WHERE file_name = \"%s\"\0", _fname.data());
	for(insert_size = 0; insert_size < 4096; insert_size++)
		if(qry[insert_size] == '\0')
			break;
	qry.resize(insert_size);
	return new q_res(db, qry.data());
}

q_res *ef_db::qry_no_exact(const std::string& _fname){
	std::string qry;
	int insert_size;
	qry.resize(4096);
	sprintf(&qry[0], "SELECT file_name, file_type, file_time FROM files WHERE file_name like \"\%%s\%\"\0", _fname.data());
	for(insert_size = 0; insert_size < 4096; insert_size++)
		if(qry[insert_size] == '\0')
			break;
	qry.resize(insert_size);
	return new q_res(db, qry.data());
}

int ef_db::add(file_info info){
	if(info.file_name[0]=='/'&&info.file_name[1]=='/')
		for(int i=1;i<info.file_name.size();i++)
			info.file_name[i-1]=info.file_name[i];
	std::string qry;
	int insert_size;
	qry.resize(4096);
	sprintf(&qry[0], "INSERT INTO files values(\
\"%s\",\"%d\",datetime(%ld,\"unixepoch\"))\0", info.file_name.data(), info.file_type, info.file_time);
	syslog(LOG_ERR,"SQL:%s",qry.c_str());
	for(insert_size = 0; insert_size < 4096; insert_size++)
		if(qry[insert_size]=='\0')
			break;
	qry.resize(insert_size);
	sqlite3pp::transaction xct(db);
	sqlite3pp::command cmd(db, qry.data());
	cmd.execute();
	xct.commit();
	return 0;
}

int ef_db::rm(const std::string& _fname){
	std::string qry;
	int insert_size;
	qry.resize(4096);
	sprintf(&qry[0], "DELETE FROM files WHERE file_name = \"%s\"\0", _fname.data());
	for(insert_size = 0; insert_size < 4096; insert_size++)
		if(qry[insert_size] == '\0')
			break;
	qry.resize(insert_size);
	db.execute(qry.data());
	return 0;
}

int ef_db::mv(const std::string&, const std::string&){
	return 1;
}
