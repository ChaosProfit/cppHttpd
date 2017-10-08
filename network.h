/*
 * mhttpd.h
 *
 *  Created on: Sep 29, 2017
 *      Author: luguanglong
 */

#ifndef NETWORK_H_
#define NETWORK_H_

#include "ret.h"
#include <sys/epoll.h>
#include <stddef.h>
#include <string>
#include "request.h"
#include "response.h"
#include "config.h"
#include "fastcgi.h"


using std::string;

class Network : public Ret{
private:
	friend class Config;

	int		i_listen_socket = 0;

	const int LISTEN_EPOLL_MAX_EVENTS = 10;
	struct	epoll_event st_listen_epoll_event = {0};
	struct	epoll_event *pst_listen_epoll_events = NULL;
	int		i_listen_epoll_fd = 0;

	int epoll_init(int i_socket);
	int opt_read(int argc, char **argv);
	int socket_init(int i_server_port);

public:
	int request_process(Request& req, Response& rsp, Config& conf, Fastcgi& fcgi);

	Network(int i_server_port, string& root_dir);
	~Network();
};

#endif /* NETWORK_H_ */
