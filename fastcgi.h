/*
 * fastcgi.h
 *
 *  Created on: Oct 4, 2017
 *      Author: luguanglong
 */

#ifndef FASTCGI_H_
#define FASTCGI_H_

#include "ret.h"
#include "config.h"
#include "request.h"

/*
 * Listening socket file number
 */
#define FCGI_LISTENSOCK_FILENO 0

typedef struct {
    unsigned char version;
    unsigned char type;
    unsigned char requestIdB1;
    unsigned char requestIdB0;
    unsigned char contentLengthB1;
    unsigned char contentLengthB0;
    unsigned char paddingLength;
    unsigned char reserved;
} FCGI_Header;

/*
 * Number of bytes in a FCGI_Header.  Future versions of the protocol
 * will not reduce this number.
 */
#define FCGI_HEADER_LEN  8

/*
 * Value for version component of FCGI_Header
 */
#define FCGI_VERSION_1           1

/*
 * Values for type component of FCGI_Header
 */
#define FCGI_BEGIN_REQUEST       1
#define FCGI_ABORT_REQUEST       2
#define FCGI_END_REQUEST         3
#define FCGI_PARAMS              4
#define FCGI_STDIN               5
#define FCGI_STDOUT              6
#define FCGI_STDERR              7
#define FCGI_DATA                8
#define FCGI_GET_VALUES          9
#define FCGI_GET_VALUES_RESULT  10
#define FCGI_UNKNOWN_TYPE       11
#define FCGI_MAXTYPE (FCGI_UNKNOWN_TYPE)

/*
 * Value for requestId component of FCGI_Header
 */
#define FCGI_NULL_REQUEST_ID     0

typedef struct {
    unsigned char roleB1;
    unsigned char roleB0;
    unsigned char flags;
    unsigned char reserved[5];
} FCGI_BeginRequestBody;

typedef struct {
    FCGI_Header header;
    FCGI_BeginRequestBody body;
} FCGI_BeginRequestRecord;

typedef struct {
    FCGI_Header header;
    char param[1024];
} FCGI_ParamsRecord;
/*
 * Mask for flags component of FCGI_BeginRequestBody
 */
#define FCGI_KEEP_CONN  1

/*
 * Values for role component of FCGI_BeginRequestBody
 */
#define FCGI_RESPONDER  1
#define FCGI_AUTHORIZER 2
#define FCGI_FILTER     3

typedef struct {
    unsigned char appStatusB3;
    unsigned char appStatusB2;
    unsigned char appStatusB1;
    unsigned char appStatusB0;
    unsigned char protocolStatus;
    unsigned char reserved[3];
} FCGI_EndRequestBody;

typedef struct {
    FCGI_Header header;
    FCGI_EndRequestBody body;
} FCGI_EndRequestRecord;

/*
 * Values for protocolStatus component of FCGI_EndRequestBody
 */
#define FCGI_REQUEST_COMPLETE 0
#define FCGI_CANT_MPX_CONN    1
#define FCGI_OVERLOADED       2
#define FCGI_UNKNOWN_ROLE     3

/*
 * Variable names for FCGI_GET_VALUES / FCGI_GET_VALUES_RESULT records
 */
#define FCGI_MAX_CONNS  "FCGI_MAX_CONNS"
#define FCGI_MAX_REQS   "FCGI_MAX_REQS"
#define FCGI_MPXS_CONNS "FCGI_MPXS_CONNS"

typedef struct {
    unsigned char type;
    unsigned char reserved[7];
} FCGI_UnknownTypeBody;

typedef struct {
    FCGI_Header header;
    FCGI_UnknownTypeBody body;
} FCGI_UnknownTypeRecord;

class Fastcgi:public Ret{
private:
	char *fcgi_ipc_path = (char *)"/var/run/cppHttpd/fcgi/unix_domain_path";
	pid_t i_fcgi_pid = 0;
	int	i_socket_fd = 0;
	unsigned short us_fcgi_id = 0;

	string	script_name;
	string	script_file_name;


	int daemon_init();
	int fcgi_module_start(Config& conf);
	int fcgi_head_build(FCGI_Header *pst_head, unsigned short us_fcgi_id, unsigned char type, unsigned short us_content_len, unsigned char uc_padding_len);

	char* fcgi_key_value_pair_build(char *pc_ptr, char *name, const char *value);
	char* fcgi_key_value_pair_build_int(char *pc_ptr, char *name, const int value);

	int fcgi_param_build(char *pc_body, Request& req, Config& conf, int *pi_content_len, int *pi_padding_len);
	int fcgi_stdin_build();

public:
	int get_cur_fcgi_id();
	int get_new_fcgi_id();

	int socket_init();
	int socket_exit();
	int send_request(Request& req, Config& conf, string& script_rela_path);
	int rcv_response();

	Fastcgi(Config& conf);
	~Fastcgi();
};

extern int get_fcgi_pid();
#endif /* FASTCGI_H_ */
