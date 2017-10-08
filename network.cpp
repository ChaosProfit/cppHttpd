/*
 * mhttpd.cpp
 *
 *  Created on: Sep 29, 2017
 *      Author: luguanglong
 */
#include "network.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <string>
#include <iostream>

#include "request.h"
#include "response.h"

int Network::epoll_init(int i_socket){
	int i_ret = 0;

	i_listen_epoll_fd = epoll_create1(0);
	if(i_listen_epoll_fd < 0){
		syslog(LOG_ERR, "listen epoll socket create fail\n");
		return FAIL;
	}

	memset(&this->st_listen_epoll_event, 0, sizeof(this->st_listen_epoll_event));
	st_listen_epoll_event.events = EPOLLIN | EPOLLET;
	st_listen_epoll_event.data.fd = i_socket;

	i_ret = epoll_ctl(i_listen_epoll_fd, EPOLL_CTL_ADD, i_socket, &st_listen_epoll_event);
	if(i_ret < 0){
		syslog(LOG_ERR, "listen epoll ctl fail\n");
		return FAIL;
	}

	pst_listen_epoll_events = (struct epoll_event *)malloc(sizeof(struct epoll_event)*LISTEN_EPOLL_MAX_EVENTS);
	if(pst_listen_epoll_events == NULL){
		syslog(LOG_ERR, "epoll events malloc fail\n");
		return FAIL;
	}

	memset(this->pst_listen_epoll_events, 0, sizeof(struct epoll_event)*LISTEN_EPOLL_MAX_EVENTS);

	return SUCCESS;
}

int Network::socket_init(int i_server_port){
	int i_ret = 0;
	int i_flags = 0;
	struct sockaddr_in	st_addr = {0};

	this->i_listen_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(this->i_listen_socket < 0){
		syslog(LOG_ERR, "socket create fail\n");
		return FAIL;
	}

	i_flags = 1;
	i_ret = setsockopt(this->i_listen_socket, SOL_SOCKET, SO_REUSEADDR, &i_flags, sizeof(i_flags));
	if(i_ret < 0){
		syslog(LOG_ERR, "reuseaddr set fail\n");
		return FAIL;
	}

	i_flags = fcntl(this->i_listen_socket, F_GETFL, 0);
	fcntl(this->i_listen_socket, F_SETFL, i_flags | O_NONBLOCK);

	memset(&st_addr, 0, sizeof(st_addr));
	st_addr.sin_family = AF_INET;
	st_addr.sin_port = htons(i_server_port);
	st_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	syslog(LOG_INFO, "server_port:%d, addr:%d\n",i_server_port, INADDR_ANY);
	i_ret = bind(this->i_listen_socket, (struct sockaddr *)&st_addr, sizeof(st_addr));
	if(i_ret < 0){
		syslog(LOG_ERR, "bind fail\n");
		return FAIL;
	}

	i_ret = listen(this->i_listen_socket, 10);
	if(i_ret < 0){
		syslog(LOG_ERR, "listen fail\n");
		return FAIL;
	}

	i_ret = this->epoll_init(this->i_listen_socket);
	if(i_ret == FAIL){
		syslog(LOG_ERR, "epoll init fail\n");
		return FAIL;
	}

	syslog(LOG_INFO, "socket create success\n");
	return SUCCESS;
}

int Network::request_process(Request& req, Response& rsp, Config& conf, Fastcgi& fcgi){
	using namespace std;
	int i_num = 0;
	int i_idx = 0;
	int				i_access_fd = 0;
	unsigned int	ui_addr_len = sizeof(struct sockaddr);
	int				i_ret = 0;

	while(1){
		i_num = epoll_wait(i_listen_epoll_fd, pst_listen_epoll_events, LISTEN_EPOLL_MAX_EVENTS, -1);
		for(i_idx = 0; i_idx < i_num; i_idx++){
			if(pst_listen_epoll_events[i_idx].data.fd != i_listen_socket){
				continue;
			}

			if( (pst_listen_epoll_events[i_idx].events & EPOLLERR)||
				(pst_listen_epoll_events[i_idx].events & EPOLLHUP)||
				!(pst_listen_epoll_events[i_idx].events & EPOLLIN)){
				continue;
			}

			i_access_fd = accept(this->i_listen_socket, (struct sockaddr *)&req.st_remote_addr, &ui_addr_len);
			if(i_access_fd < 0){
				syslog(LOG_ERR, "accept process fail");
				continue;
			}

			i_ret = recv(i_access_fd, req.get_request_buf(), req.get_request_maxsize(), 0);
			if(i_ret <= 0){
				syslog(LOG_ERR, "file:%s,line:%d,rcv fail\n",__FILE__,__LINE__);
				close(i_access_fd);
				continue;
			}

			req.request_parse();
			rsp.generate_send(req, conf, i_access_fd, fcgi);

			close(i_access_fd);
		}
	}

	return SUCCESS;
}

Network::Network(int i_server_port, string& root_dir){
	int i_ret = SUCCESS;

	i_ret = this->socket_init(i_server_port);
	if(i_ret == FAIL){
		syslog(LOG_ERR,"Mhttpd init socket fail\n");
		return;
	}
}

Network::~Network(){

}
