/*
 * reponse.h
 *
 *  Created on: Sep 29, 2017
 *      Author: luguanglong
 */

#ifndef RESPONSE_H_
#define RESPONSE_H_

#include "ret.h"
#include "request.h"
#include "version.h"
#include "config.h"
#include "fastcgi.h"

#define RSP_BODY_BUF_SIZE 2048

typedef enum{
	RESPONSE_STATUS_NONE = 0,

	/*success*/
	RESPONSE_STATUS_OK_200 = 200,

	/*client error*/
	RESPONSE_STATUS_BAD_REQUEST_400 = 400,
	RESPONSE_STATUS_NOT_FOUND_404 = 404,

	/*Server Error*/
	RESPONSE_STATUS_INTERNAL_SERVER_500 = 500,
} RESPONSE_STATUS_CODE;

typedef enum{
	CONTENT_TYPE_TEXT_NONE = 0,
	CONTENT_TYPE_TEXT_HTML = 1,
}CONTENT_TYPE;

typedef enum{
	HTTP_DYNAMIC_TYPE_STATIC = 1,
	HTTP_DYNAMIC_TYPE_PHP,
}HTTP_DYNAMIC_TYPE;

class Response : public Ret{
private:
	const string HTTP_VERSION = "HTTP/1.1";
	const string HTTP_CONTENT_TYPE = "Content-Type";
	const string HTTP_DATE = "Date";
	const string HTTP_CONTENT_LENGTH = "Content-Length";
	const string HTTP_SERVER = "Server";
	const string HTTP_CRLF_FLAG = "\r\n";

	const char *HTTP_FILE_404	= "404.html";
	const char *HTTP_FILE_500	= "500.html";
	const char *SUPPORT_DYNAMIC_TYPE = "([\\w|\\W]*)(\\.)(php|PHP)";

	RESPONSE_STATUS_CODE	e_status_code = RESPONSE_STATUS_NONE;
	string absolute_path;
	string rela_path;
	string response_head;
	unsigned int ui_body_len = 0;
	char	ac_body_buf[RSP_BODY_BUF_SIZE] = {0};
	HTTP_DYNAMIC_TYPE	e_dynamic_type;

	string get_date();
	string get_reason_code();
	string get_contet_type(CONTENT_TYPE e_type);
	string get_server();
	HTTP_DYNAMIC_TYPE get_dynamic_type();

	int static_response(Config& conf, int i_socket_fd);			//response to the static request
	int dynamic_response(Config& conf, Request& req, int i_socket_fd, Fastcgi& fcgi);		//response to the dynamic request
	bool file_exist_check(const char *pc_rela_path, HTTP_DYNAMIC_TYPE e_type);
	int head_generate();
	int body_generate(string& root_path);
public:
	unsigned short get_content_length();
	int generate_send(Request& req, Config& conf, int i_socket_fd, Fastcgi& fcgi);
	const string& get_head();
	const char *get_body();
	unsigned int get_length();
	string& get_rela_path();

	Response();
	~Response();
};

inline string& Response::get_rela_path(){
	return this->rela_path;
}

inline const char* Response::get_body(){
	return this->ac_body_buf;
}

inline const string& Response::get_head(){
	return this->response_head;
}

inline string Response::get_contet_type(CONTENT_TYPE e_type){
	switch(e_type){
	case CONTENT_TYPE_TEXT_HTML:
		return "text/html";
		break;
	default:
		break;
	}

	return NULL;
}

inline string Response::get_date(){
	using namespace std;
	time_t t_time;

	time(&t_time);
	string time_string = asctime(gmtime(&t_time));
	time_string.pop_back();
//	cout << "time_string\n" << time_string << endl;

	return time_string;
}

inline unsigned short Response::get_content_length(){
	return this->ui_body_len;
}

inline string Response::get_server(){
	return SERVER_NAME;
}

inline string Response::get_reason_code(){

	switch(this->e_status_code){
	case RESPONSE_STATUS_OK_200:
		return "OK";
		break;
	case RESPONSE_STATUS_BAD_REQUEST_400:
		return "Bad Request";
		break;
	case RESPONSE_STATUS_NOT_FOUND_404:
		return "Not Found";
		break;
	default:
		break;
	}

	return NULL;
}

#endif /* RESPONSE_H_ */
