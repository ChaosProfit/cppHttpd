/*
 * fastcgi.cpp
 *
 *  Created on: Oct 4, 2017
 *      Author: luguanglong
 */

#include "fastcgi.h"
#include "version.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string>
#include "config.h"
#include <iostream>
#include <string>

static int gi_fcgi_pid = 0;

int get_fcgi_pid(){
	return gi_fcgi_pid;
}

int Fastcgi::daemon_init(){
	pid_t	i_pid_tmp = 0;
	int i_ret = 0;
	int i_fd = 0;

	i_pid_tmp = fork();
	if(i_pid_tmp < 0){
		syslog(LOG_ERR, "FILE:%s,LINE:%d\n",__FILE__,__LINE__);
		return FAIL;
	}
	else if(i_pid_tmp > 0){
		return SUCCESS;
	}

	this->i_fcgi_pid = fork();
	if(this->i_fcgi_pid < 0){
		syslog(LOG_ERR, "FILE:%s,LINE:%d\n",__FILE__,__LINE__);
		return FAIL;
	}
	else if(this->i_fcgi_pid > 0){
		gi_fcgi_pid = this->i_fcgi_pid;
		return SUCCESS;
	}

	i_ret = setsid();
	if(i_ret < 0){
		syslog(LOG_ERR, "FILE:%s,LINE:%d\n",__FILE__,__LINE__);
		return FAIL;
	}

	chdir("/");
	for(i_fd = 0; i_fd < 10; i_fd++){
		close(i_fd);
	}

	open("/dev/null", O_RDONLY);
	open("/dev/null", O_RDWR);
	open("/dev/null", O_RDWR);

	return SUCCESS;
}

int Fastcgi::fcgi_module_start(Config& conf){
	using namespace std;
	int i_fd = 0;
	int i_ret = 0;

	this->i_fcgi_pid = fork();
	if(this->i_fcgi_pid < 0){
		syslog(LOG_ERR, "FILE:%s,LINE:%d\n",__FILE__,__LINE__);
		return FAIL;
	}
	else if(this->i_fcgi_pid > 0){
		return SUCCESS;
	}

	if(setsid() < 0){
		syslog(LOG_ERR, "FILE:%s,LINE:%d\n",__FILE__,__LINE__);
		return FAIL;
	}

//	chdir(conf.get_root_dir().data());
	for(i_fd = 0; i_fd < 10; i_fd++){
		close(i_fd);
	}

	open("/dev/null", O_RDONLY);
	open("/dev/null", O_RDWR);
	open("/dev/null", O_RDWR);

	unlink(this->fcgi_ipc_path);

	char *pc_module_path = (char *)malloc(conf.get_fcgi_module().size() + 1);
	if(pc_module_path == NULL){
		syslog(LOG_ERR, "FILE:%s,LINE:%d\n",__FILE__,__LINE__);
		return FAIL;
	}

	memset(pc_module_path, 0, (conf.get_fcgi_module().size() + 1));
	conf.get_fcgi_module().copy(pc_module_path, conf.get_fcgi_module().size(), 0);
	char *argv[] = {pc_module_path, (char *)"-b", this->fcgi_ipc_path, 0};
//	char *env[] = {(char *)"PHP_FCGI_CHILDREN=0", 0};

	setenv("PHP_FCGI_CHILDREN", "0", 1);

	i_ret = execvp(conf.get_fcgi_module().data(), argv);
	if(i_ret == -1){
		free(pc_module_path);
		syslog(LOG_ERR, "FILE:%s,LINE:%d,execv error\n",__FILE__,__LINE__);
		exit(-1);
		return FAIL;
	}

	free(pc_module_path);
	return SUCCESS;
}

int Fastcgi::socket_init(){
	struct sockaddr_un st_addr = {0};
	int i_ret = 0;

	this->i_socket_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if(this->i_socket_fd < 0){
		return FAIL;
	}

	st_addr.sun_family = AF_UNIX;
	strncpy(st_addr.sun_path, this->fcgi_ipc_path, strlen(this->fcgi_ipc_path));

	i_ret = connect(this->i_socket_fd, (struct sockaddr *)&st_addr, sizeof(st_addr));
	if(i_ret < 0){
		syslog(LOG_ERR, "FILE:%s,LINE:%d,connect error\n",__FILE__,__LINE__);
		return FAIL;
	}

	return SUCCESS;
}

int Fastcgi::socket_exit(){
	close(this->i_socket_fd);

	return SUCCESS;
}

int Fastcgi::fcgi_head_build(FCGI_Header *pst_head, unsigned short us_fcgi_id, unsigned char type, unsigned short us_content_len, unsigned char uc_padding_len){
	pst_head->version = FCGI_VERSION_1;
	pst_head->type = type;
	pst_head->requestIdB1 = ((us_fcgi_id << 8) & 0xFF);
	pst_head->requestIdB0 = (us_fcgi_id & 0xFF);
	pst_head->contentLengthB1 = ((us_content_len << 8) & 0xFF);
	pst_head->contentLengthB0 = us_content_len & 0xFF;
	pst_head->paddingLength = uc_padding_len;

	return SUCCESS;
}

char* Fastcgi::fcgi_key_value_pair_build(char *pc_ptr, char *name, const char *value){
	using namespace::std;

	int i_name_len = strlen(name);
	int i_value_len = strlen(value);

	cout << "name:" << string(name) << "value:" << string(value) << endl;
	if(i_name_len < 127){
		*pc_ptr++ = (i_name_len & 0xff);
	}
	else{
		*pc_ptr++ = ((i_name_len << 24) & 0xff) | 0x80;
		*pc_ptr++ = ((i_name_len << 16) & 0xff);
		*pc_ptr++ = ((i_name_len << 8) & 0xff);
		*pc_ptr++ = (i_name_len & 0xff);
	}

	if(i_value_len < 127){
		*pc_ptr++ = (i_value_len & 0xff);
	}
	else{
		*pc_ptr++ = ((i_value_len << 24) & 0xff) | 0x80;
		*pc_ptr++ = ((i_value_len << 16) & 0xff);
		*pc_ptr++ = ((i_value_len << 8) & 0xff);
		*pc_ptr++ = (i_value_len & 0xff);
	}

	while(i_name_len-- > 0){
		*pc_ptr++ = *name++;
	}

	while(i_value_len-- > 0){
			*pc_ptr++ = *value++;
	}

	return pc_ptr;
}

char* Fastcgi::fcgi_key_value_pair_build_int(char *pc_ptr, char *name, const int value){
	using namespace std;
	char	ac_value[48] = {0};
	int i_name_len = strlen(name);
	int i_value_len = 0;
	int i_idx = 0;

	snprintf(ac_value, 48, "%d", value);
	i_value_len = strlen(ac_value);

	if(i_name_len < 127){
		*pc_ptr++ = (i_name_len & 0xff);
	}
	else{
		*pc_ptr++ = ((i_name_len << 24) & 0xff) | 0x80;
		*pc_ptr++ = ((i_name_len << 16) & 0xff);
		*pc_ptr++ = ((i_name_len << 8) & 0xff);
		*pc_ptr++ = (i_name_len & 0xff);
	}

	if(i_value_len < 127){
		*pc_ptr++ = (i_value_len & 0xff);
	}
	else{
		*pc_ptr++ = ((i_value_len << 24) & 0xff) | 0x80;
		*pc_ptr++ = ((i_value_len << 16) & 0xff);
		*pc_ptr++ = ((i_value_len << 8) & 0xff);
		*pc_ptr++ = (i_value_len & 0xff);
	}

	while(i_name_len-- > 0){
		*pc_ptr++ = *name++;
	}

	while(i_value_len-- > 0){
		*pc_ptr++ = ac_value[i_idx++];
	}

	return pc_ptr;
}

int Fastcgi::fcgi_param_build(char *pc_body, Request& req, Config& conf, int *pi_content_len, int *pi_padding_len){
	using namespace std;
	char *pc_head = pc_body;
	char *pc_end = NULL;

	pc_end = this->fcgi_key_value_pair_build_int(pc_head, (char *)"CONTENT_LENGTH", 0);
	pc_end = this->fcgi_key_value_pair_build(pc_end, (char *)"QUERY_STRING", "");
	pc_end = this->fcgi_key_value_pair_build(pc_end, (char *)"REQUEST_URI", req.get_url().data());
	pc_end = this->fcgi_key_value_pair_build_int(pc_end, (char *)"REDIRECT_STATUS", 200);
	pc_end = this->fcgi_key_value_pair_build(pc_end, (char *)"SCRIPT_NAME", this->script_name.data());
	pc_end = this->fcgi_key_value_pair_build(pc_end, (char *)"SCRIPT_FILENAME", this->script_file_name.data());
	pc_end = this->fcgi_key_value_pair_build(pc_end, (char *)"DOCUMENT_ROOT", conf.get_root_dir().data());
	pc_end = this->fcgi_key_value_pair_build(pc_end, (char *)"REQUEST_METHOD", req.get_method_str());
	pc_end = this->fcgi_key_value_pair_build(pc_end, (char *)"SERVER_PROTOCOL", "HTTP/1.1");
	pc_end = this->fcgi_key_value_pair_build(pc_end, (char *)"SERVER_SOFTWARE", SERVER_NAME);
	pc_end = this->fcgi_key_value_pair_build(pc_end, (char *)"GATEWAY_INTERFACE", CGI_INTERFACE);
	pc_end = this->fcgi_key_value_pair_build_int(pc_end, (char *)"SERVER_PORT", conf.get_server_port());
	pc_end = this->fcgi_key_value_pair_build(pc_end, (char *)"SERVER_ADDR", conf.get_server_ip());
	pc_end = this->fcgi_key_value_pair_build(pc_end, (char *)"SERVER_NAME", conf.get_server_ip());
	pc_end = this->fcgi_key_value_pair_build(pc_end, (char *)"REMOTE_ADDR", req.get_remote_ip());
	pc_end = this->fcgi_key_value_pair_build_int(pc_end, (char *)"REMOTE_PORT", req.get_remote_port());

	*pi_content_len = pc_end - pc_head - 1;
	*pi_padding_len = 8 - (*pi_content_len%8);

	return SUCCESS;
}

int Fastcgi::fcgi_stdin_build()
{

	return SUCCESS;
}

static void debug(char *pc_data, int i_len){
	FILE	*pf_file = NULL;

	pf_file = fopen("cppHttpd.test", "a");
	if(pf_file != NULL){
		fwrite(pc_data, 1, i_len, pf_file);
		fflush(pf_file);
		fclose(pf_file);
	}
}

int Fastcgi::send_request(Request& req, Config& conf, string& script_rela_path){
	using namespace std;

	int i_ret = 0;
	int i_content_len = 0;
	int i_padding_len = 0;
	FCGI_BeginRequestRecord st_brr = {0};
	FCGI_ParamsRecord		st_para = {0};

	++this->us_fcgi_id;

	this->fcgi_head_build(&st_brr.header, this->us_fcgi_id, FCGI_BEGIN_REQUEST,  sizeof(st_brr.body), 0);
	st_brr.body.flags = 0;
	st_brr.body.roleB1 = 0;
	st_brr.body.roleB0 = FCGI_RESPONDER;

	i_ret = send(this->i_socket_fd, &st_brr, sizeof(st_brr), 0);
	if(i_ret < 0){
		syslog(LOG_ERR, "FILE:%s,LINE:%d fcgi send request error\n",__FILE__,__LINE__);
		return FAIL;
	}

	memset(&st_para, 0, sizeof(st_para));

	this->script_name = script_rela_path;
	this->script_file_name = conf.get_root_dir() +  script_rela_path;

	this->fcgi_param_build(st_para.param, req, conf, &i_content_len, &i_padding_len);
	this->fcgi_head_build(&st_para.header, this->us_fcgi_id, FCGI_PARAMS, i_content_len, i_padding_len);
	i_ret = send(this->i_socket_fd, &st_para, (sizeof(st_para.header) + i_content_len + i_padding_len), 0);
	if(i_ret < 0){
		syslog(LOG_ERR, "FILE:%s,LINE:%d fcgi send param error\n",__FILE__,__LINE__);
		return FAIL;
	}

	memset(&st_para, 0, sizeof(st_para));
	this->fcgi_stdin_build();
	this->fcgi_head_build(&st_para.header, this->us_fcgi_id, FCGI_STDIN, 0,0);
	i_ret = send(this->i_socket_fd, &st_para, sizeof(st_para.header), 0);
	if(i_ret < 0){
		syslog(LOG_ERR, "FILE:%s,LINE:%d fcgi send stdin error\n",__FILE__,__LINE__);
		return FAIL;
	}

	cout << "file:" << __FILE__<< "line:" << to_string(__LINE__) << endl;
	return SUCCESS;
}

int Fastcgi::rcv_response(){
	using namespace std;
	FCGI_Header	st_header = {0};
	int i_ret = 0;
	unsigned short us_content_len = 0;
	unsigned char uc_padding_len = 0;
	char *pc_data = NULL;

	while(1){

		i_ret = recv(this->i_socket_fd, &st_header, sizeof(st_header), 0);
		if(i_ret < 0){
			syslog(LOG_ERR, "file:%s,line:%dfcgi rcv err\n",__FILE__,__LINE__);
			break;
		}

		syslog(LOG_INFO, "fcgi_rcv_type:%d\n", st_header.type);
		if((st_header.type == FCGI_STDOUT) || (st_header.type == FCGI_STDERR)){
			us_content_len = (st_header.contentLengthB1 << 8)|st_header.contentLengthB0;
			uc_padding_len = st_header.paddingLength;
			pc_data = (char *)malloc(us_content_len + uc_padding_len);
			if(pc_data == NULL){
				return FAIL;
			}

			i_ret = recv(this->i_socket_fd, pc_data, (us_content_len + uc_padding_len), 0);
			if(i_ret < 0){
				syslog(LOG_ERR, "file:%s,line:%dfcgi rcv err\n",__FILE__,__LINE__);
				break;
			}
		}
		else if(st_header.type == FCGI_END_REQUEST){
			break;
		}
	}

	free(pc_data);
	return SUCCESS;
}

Fastcgi::Fastcgi(Config& conf){
	using namespace std;

	this->us_fcgi_id = 0;

	this->fcgi_module_start(conf);

	return;
}

Fastcgi::~Fastcgi(){

}
