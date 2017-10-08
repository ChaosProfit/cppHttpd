/*
 * response.cpp
 *
 *  Created on: Sep 29, 2017
 *      Author: luguanglong
 */
#include <fstream>
#include <iostream>
#include <string>
#include "response.h"
#include "version.h"
#include "config.h"
#include <chrono>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <syslog.h>
#include <regex>
#include <unistd.h>

Response::Response(){
	this->response_head.reserve(RSP_BODY_BUF_SIZE);
}

Response::~Response(){

}

HTTP_DYNAMIC_TYPE Response::get_dynamic_type(){
	using namespace std;
	smatch	match;

	if(regex_search(this->rela_path, match, regex(SUPPORT_DYNAMIC_TYPE)) == false){
		cout << "file:" << __FILE__ << "line:" << to_string(__LINE__) <<"\n" <<endl;
		return HTTP_DYNAMIC_TYPE_STATIC;
	}

	return HTTP_DYNAMIC_TYPE_PHP;
}

int Response::head_generate(){
	using namespace std;
 	this->response_head.erase();

	this->response_head.append(HTTP_VERSION + " " + to_string(this->e_status_code) + " " + this->get_reason_code() + HTTP_CRLF_FLAG);
	this->response_head.append(HTTP_CONTENT_TYPE + ":" + this->get_contet_type(CONTENT_TYPE_TEXT_HTML) + HTTP_CRLF_FLAG);
	this->response_head.append(HTTP_DATE + ":" + this->get_date() + HTTP_CRLF_FLAG);
	this->response_head.append(HTTP_CONTENT_LENGTH + ":" + to_string(this->ui_body_len) + HTTP_CRLF_FLAG);
	this->response_head.append(HTTP_SERVER + ":" + this->get_server() + HTTP_CRLF_FLAG);
	this->response_head.append(HTTP_CRLF_FLAG);

	return SUCCESS;
}

bool Response::file_exist_check(const char* pc_rela_path, HTTP_DYNAMIC_TYPE e_type){
	using namespace std;

	struct stat st_state = {0};

	if((stat(this->rela_path.data(), &st_state) == 0) && (st_state.st_size > 0)){
		if(e_type == HTTP_DYNAMIC_TYPE_STATIC){
			this->e_status_code = RESPONSE_STATUS_OK_200;
			this->ui_body_len = st_state.st_size;
		}

		return true;
	}
	else{
		struct stat st_state2 = {0};

		this->e_status_code = RESPONSE_STATUS_NOT_FOUND_404;
		this->rela_path = HTTP_FILE_404;
		stat(this->rela_path.data(), &st_state2);
		this->ui_body_len = st_state2.st_size;

		return false;
	}

	return true;
}

int Response::static_response(Config& conf, int i_socket_fd){
	FILE	*pf_file = NULL;
	int		i_read_len = 0;
	int		i_send_len = 0;
	int		i_ret = 0;

	this->file_exist_check(this->rela_path.data(), HTTP_DYNAMIC_TYPE_STATIC);

	//pf_file = fopen(this->absolute_path.data(), "r");
	pf_file = fopen(this->rela_path.data(), "r");
	if(pf_file == NULL){
		syslog(LOG_ERR, "%s file open fail\n", this->absolute_path.data());
		return FAIL;
	}

	this->head_generate();
	i_ret = send(i_socket_fd, this->response_head.data(), this->response_head.length(), 0);
	if(i_ret < (int)this->response_head.length()){
		fclose(pf_file);
		syslog(LOG_ERR, "send error\n");
		return FAIL;
	}

	while(1){
		i_read_len = fread(this->ac_body_buf, 1, RSP_BODY_BUF_SIZE, pf_file);
		if(i_read_len < RSP_BODY_BUF_SIZE){
			this->ac_body_buf[i_read_len] = 0;
		}

		i_send_len = send(i_socket_fd, this->ac_body_buf, i_read_len, 0);
		if(i_send_len != i_read_len){
			syslog(LOG_ERR, "rsp body send err\n");
		}

		if(i_read_len < RSP_BODY_BUF_SIZE){
			break;
		}
	}

	fclose(pf_file);

	return SUCCESS;
}

int Response::dynamic_response(Config& conf, Request& req,int i_socket_fd, Fastcgi& fcgi){
	using namespace std;

	if(this->file_exist_check(this->rela_path.data(), HTTP_DYNAMIC_TYPE_PHP) == false){
		return this->static_response(conf, i_socket_fd);
	}

	cout <<"file:" << __FILE__ << "line:" << to_string(__LINE__) << endl;

	fcgi.socket_init();
	fcgi.send_request(req, conf, this->rela_path);
	usleep(500000);
	fcgi.rcv_response();
	fcgi.socket_exit();


	return SUCCESS;
}

int Response::generate_send(Request& req, Config& conf, int i_socket_fd, Fastcgi& fcgi){
	using namespace std;

	if(req.get_method() == HTTP_METHOD_GET){
		if(req.get_raw_path() == "/"){
			this->absolute_path = conf.get_root_dir() + conf.get_index_file();
			this->rela_path = conf.get_index_file();
		}
		else{
			this->absolute_path = conf.get_root_dir() + req.get_raw_path();
			this->rela_path = req.get_raw_path();
		}

		if(this->get_dynamic_type() == HTTP_DYNAMIC_TYPE_STATIC){
			this->static_response(conf, i_socket_fd);
		}
		else{
			cout <<"file:" << __FILE__ << "line:" << to_string(__LINE__) << endl;
			this->dynamic_response(conf, req, i_socket_fd, fcgi);
		}
	}
	else if(req.get_method() == HTTP_METHOD_POST){

	}
	else{
		this->e_status_code = RESPONSE_STATUS_BAD_REQUEST_400;
	}

	return SUCCESS;
}
