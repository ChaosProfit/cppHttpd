/*
 * config.cpp
 *
 *  Created on: Sep 29, 2017
 *      Author: luguanglong
 */
#include "config.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <regex>
#include <fstream>
#include <iostream>

using std::string;

int Config::get_value(string& input, const char *name, string& value){
	using namespace std;
	smatch	match;

	if(regex_search(input, match, regex(name)) == false){
		syslog(LOG_ERR, "%s value do not exist\n", name);
		return FAIL;
	}

	value = match[3].str();
	return SUCCESS;
}

int Config::get_value_int(string& input,const char *name, int *value){
	string	tmp;

	this->get_value(input, name, tmp);

	*value = atoi(tmp.c_str());

	return SUCCESS;
}

int Config::fcgi_mpodule_process(){
	using namespace std;
/*
	for(auto module: this->support_fcgi_module){
		if(this->fcgi_module.find(module) != std::string::npos)
			if(this->fcgi_module.find("/usr/bin") == std::string::npos){
					this->fcgi_module.insert(0, "/usr/bin/");
			}
		}
	}
*/
	this->fcgi_module = "php-cgi";
/*
	if(this->fcgi_module.find("/usr/bin") == std::string::npos){
		this->fcgi_module.insert(0, "/usr/bin/");
		;
	}
*/
	return SUCCESS;
}

Config::Config(const char *file_path){
	using namespace std;
	struct stat st_stat = {0};
	int i_ret = 0;
	char *pc_config_buf = NULL;
	ifstream config_file;

	i_ret = stat(file_path, &st_stat);
	if(i_ret < 0){
		syslog(LOG_ERR, "config file %s not found\n",file_path);
		return;
	}

	if(st_stat.st_size > CONFIG_FILE_MAX_SIZE){
		syslog(LOG_ERR, "config file %s too large\n", file_path);
		return;
	}

	pc_config_buf = (char *)malloc(st_stat.st_size + 1);
	if(pc_config_buf == NULL){
		syslog(LOG_ERR, "config buffer malloc fail\n");
		return;
	}

	config_file.open(file_path);
	if(config_file.is_open() == false){
		free(pc_config_buf);
		syslog(LOG_ERR, "config file %s open fail\n", file_path);
		return;
	}

	config_file.read(pc_config_buf, st_stat.st_size);
	config_file.clear();
	config_file.close();

	string config_buf(pc_config_buf);

	this->get_value(config_buf, ROOT_PATH_NAME, root_path);
	this->get_value_int(config_buf, SERVER_PORT_NAME, &i_server_port);
	this->get_value(config_buf, INDEX_FILE, index_file);
	this->get_value(config_buf, FCGI_MODULE, fcgi_module);

	this->get_index_file();

	if(this->root_path.compare((this->root_path.size()-1), 1, "/") != 0){
		this->root_path.append("/");
	}

	this->fcgi_mpodule_process();

	free(pc_config_buf);
	return;
}

string& Config::get_index_file(){
	using namespace std;
	struct stat st_state = {0};

	if((this->real_index_file.empty() == false) && (stat(this->real_index_file.data(), &st_state) == 0) && (st_state.st_size > 0)){
		return this->real_index_file;
	}

	smatch	match;
	auto pos = this->index_file.cbegin();
	auto end = this->index_file.cend();

	for( ; regex_search(pos, end, match, regex("([ ]{0,}[\\w|_|-]+\\.)(html|php)([\\,]{0,1})")); pos = match.suffix().first) {
			string tmp = match[1].str() + match[2].str();
			if((stat(tmp.data(), &st_state) == 0) && (st_state.st_size > 0)){
				this->real_index_file = tmp;
				return this->real_index_file;
			}

	}

	this->real_index_file = "";
	return this->real_index_file;
}

Config::~Config(){

}
