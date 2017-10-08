/*
 * request.cpp
 *
 *  Created on: Sep 30, 2017
 *      Author: luguanglong
 */

#include <syslog.h>
#include <regex>
#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <stdio.h>

#include "request.h"
#include "ret.h"

char *Request::get_request_buf(){
	return this->ac_request_buf;
}

int Request::get_request_maxsize(){
	return REQUEST_BUF_SIZE;
}

HTTP_REQUEST_METHOD Request::get_method(){
	return this->e_method;
}

const char* Request::get_method_str(){
	switch(this->e_method){
	case HTTP_METHOD_GET:
		return "GET";
		break;
	case HTTP_METHOD_POST:
		return "POST";
		break;
	default:
		break;
	}

	return NULL;
}

string& Request::get_raw_path(){
	return this->raw_path;
}

int Request::url_process(){
	using namespace::std;
	smatch match;

	if(regex_search(this->url, match, regex(URL_REGEX.data())) == false){
		syslog(LOG_ERR, "file:%s,line:%d,url not match\n",__FILE__,__LINE__);
		return FAIL;
	}

	this->raw_path = match[2].str();

	return SUCCESS;
}

int Request::start_line_process(){
	unsigned short us_method_end = 0;
	unsigned short us_uri_end = 0;

	us_method_end = this->start_line.find(' ', 0);
	us_uri_end = this->start_line.find(' ', (us_method_end + 1));

	if(this->start_line.find("GET", 0, us_method_end) != std::string::npos){
		this->e_method = HTTP_METHOD_GET;
	}
	else if(this->start_line.find("POST", 0, us_method_end) != std::string::npos){
		this->e_method = HTTP_METHOD_POST;
	}
	else{
		syslog(LOG_ERR, "request method %s not support yet\n",this->start_line.substr(0, us_method_end).data());
		return FAIL;
	}

	this->url = this->start_line.substr((us_method_end + 1), (us_uri_end - us_method_end - 1));

	this->url_process();

	return SUCCESS;
}

int Request::request_parse(){
	using namespace std;

	unsigned short us_start_line_end = 0;

	this->us_remote_port = ntohs(this->st_remote_addr.sin_port);
	snprintf(this->ac_remote_ip, 20, "%s", inet_ntoa(this->st_remote_addr.sin_addr));

	this->request = this->ac_request_buf;

	us_start_line_end = this->request.find(HTTP_CRLF_FLAG);
	this->start_line = this->request.substr(0, us_start_line_end);

	this->start_line_process();

	return SUCCESS;
}

int Request::get_remote_port(){
	return this->us_remote_port;
}

char* Request::get_remote_ip(){
	return this->ac_remote_ip;
}

Request::Request(){
	this->request.reserve(REQUEST_BUF_SIZE);
}

Request::~Request(){

}
