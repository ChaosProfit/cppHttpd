/*
 * request.h
 *
 *  Created on: Sep 30, 2017
 *      Author: luguanglong
 */

#ifndef REQUEST_H_
#define REQUEST_H_

#include "ret.h"
#include <string>
#include <arpa/inet.h>
#include <netinet/in.h>

using std::string;

#define REQUEST_BUF_SIZE	1536

typedef enum {
	HTTP_METHOD_NONE = 0,
	HTTP_METHOD_GET,
	HTTP_METHOD_POST,
	HTTP_METHOD_HEAD,
	HTTP_METHOD_PUT,
	HTTP_METHOD_DELETE,
	HTTP_METHOD_CONNECT,
} HTTP_REQUEST_METHOD;

/*
typedef enum{
	CONNECT_STATE_ERROR = -1,
	CONNECT_STATE_CLOSE = 0,
	CONNECT_STATE_READ,
	CONNECT_STATE_WRITE,
} CONNECT_STATE;
*/

class Request : public Ret{
private:
	const string HTTP_CRLF_FLAG = "\r\n";
	const string URL_REGEX = "([http://|https://|ftp://]{0,1})(/[\\w|\\W]{0,})";

	//CONNECT_STATE 	e_state = CONNECT_STATE_CLOSE;
	HTTP_REQUEST_METHOD 	e_method = HTTP_METHOD_NONE;

	char			ac_request_buf[REQUEST_BUF_SIZE];
	string			request;

	string			start_line;
	string			url;
	string			raw_path;

	unsigned int	ui_socket_fd = 0;

	unsigned short	us_remote_port = 0;
	char			ac_remote_ip[20] = {0};
	int		start_line_process();
	int		url_process();
public:
	struct sockaddr_in st_remote_addr;

	int		request_parse();
	char	*get_request_buf();
	string& get_raw_path();
	string& get_url();
	int get_remote_port();
	char* get_remote_ip();
	int		get_request_maxsize();
	HTTP_REQUEST_METHOD get_method();
	const char* get_method_str();

	Request();
	~Request();
};

inline string& Request::get_url(){
	return this->url;
}

#endif /* REQUEST_H_ */
