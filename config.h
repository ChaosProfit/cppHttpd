/*
 * config.h
 *
 *  Created on: Sep 29, 2017
 *      Author: luguanglong
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include "ret.h"
#include <string>

using std::string;

class Config : public Ret{
private:
	const char	*ROOT_PATH_NAME = "(root_dir[ ]{0,}=)([ ]{0,})(/[\\w|_|/]*)";
	const char	*SERVER_PORT_NAME = "(port[ ]{0,}=)([ ]{0,})([\\d]+)";
	const char	*INDEX_FILE = "(index_file[ ]{0,}=)([ ]{0,})([\\w|\\W]+)";
	const char	*FCGI_MODULE = "(fastcgi_module[ ]{0,}=)([ ]{0,})([\\w|-]+)";
	const int	CONFIG_FILE_MAX_SIZE = 4096;
	const int 	VALUE_MAX_SIZE	= 128;
	char *test = (char *)"(\\{)([ ]{0,}[\\w|_|-].[html|php]\\,{0,1})(\\})";
	string	config_path;
	string	root_path;
	string	index_file;
	string	real_index_file;
	string	fcgi_module;
	int		i_server_port = 80;

	int fcgi_mpodule_process();

	int get_value_int(string& input,const char *name, int *value);
	int get_value(string& input,const char *name, string& value);
public:
	int get_server_port();
	char* get_server_ip();
	string& get_root_dir();
	string& get_index_file();
	string& get_fcgi_module();

	Config(const char *path);
	~Config();
};

inline int Config::get_server_port(){
	return this->i_server_port;
}

inline string& Config::get_root_dir(){
	return this->root_path;
}

inline string& Config::get_fcgi_module(){
	return this->fcgi_module;
}

inline char* Config::get_server_ip(){
	return (char *)"127.0.0.1";
}
#endif /* CONFIG_H_ */
