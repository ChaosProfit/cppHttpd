/*
 * microHttpd.cpp
 *
 *  Created on: Sep 29, 2017
 *      Author: luguanglong
 */
#include "cppHttpd.h"

#include <string>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include "config.h"
#include "network.h"
#include "request.h"
#include "fastcgi.h"

using std::string;

int opt_read(int argc, char **argv, string& path){
	int i_opt = 0;

	while(1){
		i_opt = getopt(argc, argv, "c:");
		if(i_opt < 0){
			break;
		}

		switch(i_opt){
		case 'c':
			path.append(optarg, OPT_MAX_LEN);
			break;
		default:
			break;
		}
	}

	return 0;
}

static void cppHttpd_exit(){

	syslog(LOG_INFO,"kill process %d\n", get_fcgi_pid());
	kill(get_fcgi_pid(), SIGTERM);

	/*wait for children process exit*/
	sleep(1);

	exit(0);
}

void signal_process(int sig){
	using namespace::std;
	switch(sig){
	case SIGPIPE:
		syslog(LOG_INFO, "file:%s,line:%d,catch a SIGPIPE signal\n", __FILE__,__LINE__);
		break;
	case SIGTERM:
		syslog(LOG_INFO, "file:%s,line:%d,catch a SIGTERM signal\n", __FILE__,__LINE__);
		cppHttpd_exit();
		break;
	case SIGINT:
		syslog(LOG_INFO, "file:%s,line:%d,catch a SIGINT signal\n", __FILE__,__LINE__);
		cppHttpd_exit();
		break;
	case SIGCHLD:
		waitpid(get_fcgi_pid(), NULL, WNOHANG);
		syslog(LOG_INFO, "file:%s,line:%d,catch a SIGCHLD signal\n", __FILE__,__LINE__);
		break;
	default:
		break;
	}
}

void signal_init(){
	struct sigaction st_act = {0};

	st_act.sa_flags = 0;
	st_act.sa_handler = signal_process;
	if(sigemptyset(&st_act.sa_mask) != 0){
		syslog(LOG_ERR, "file:%s,line:%d,sigaction error\n", __FILE__,__LINE__);
	}

	if(sigaction(SIGPIPE, &st_act, NULL) != 0){
		syslog(LOG_ERR, "file:%s,line:%d,sigaction error\n", __FILE__,__LINE__);
	}

	if(sigaction(SIGCHLD, &st_act, NULL) != 0){
		syslog(LOG_ERR, "file:%s,line:%d,sigaction error\n", __FILE__,__LINE__);
	}

	if(sigaction(SIGINT, &st_act, NULL) != 0){
		syslog(LOG_ERR, "file:%s,line:%d,sigaction error\n", __FILE__,__LINE__);
	}

	if(sigaction(SIGTERM, &st_act, NULL) != 0){
		syslog(LOG_ERR, "file:%s,line:%d,sigaction error\n", __FILE__,__LINE__);
	}
}

void dir_init(){
	DIR *pd_var_dir = NULL;

	pd_var_dir = opendir("/var/run/cppHttpd/fcgi");
	if(pd_var_dir == NULL){
		if(mkdir("/var/run/cppHttpd", 00777) < 0){
			syslog(LOG_ERR, "file:%s,line:%d,mkdir error\n", __FILE__,__LINE__);
		}
		if(mkdir("/var/run/cppHttpd/fcgi", 00777) < 0){
			syslog(LOG_ERR, "file:%s,line:%d,mkdir error\n", __FILE__,__LINE__);
		}
	}
	else{
		closedir(pd_var_dir);
	}

}

int daemon_init(const char *parent_dir){
	pid_t i_pid1 = 0;
	pid_t i_pid2 = 0;
	int i_fd = 0;
	struct rlimit st_limit = {0};

	umask(0);

	if(getrlimit(RLIMIT_NOFILE, &st_limit) < 0){
		syslog(LOG_ERR, "file:%s,line:%d, getrlimit error\n",__FILE__,__LINE__);
	}
	i_pid1 = fork();
	if(i_pid1 < 0){
		return -1;
	}
	else if(i_pid1 > 0){
		/*The first parent exit*/
		exit(0);
	}

	/*ensure the first process exit firstly*/
	sleep(1);

	setsid();
	signal_init();

	i_pid2 = fork();
	if(i_pid2 < 0){
		return -1;
	}
	else if(i_pid2 > 0){
		exit(0);
	}

	chdir(parent_dir);

	if(st_limit.rlim_max == RLIM_INFINITY){
		st_limit.rlim_max = 1024;
	}
	for(i_fd = 0; i_fd < st_limit.rlim_max; i_fd++){
		close(i_fd);
	}

	open("/dev/null", O_RDONLY);
	open("/dev/null", O_RDWR);
	open("/dev/null", O_RDWR);

	openlog(SERVER_NAME, LOG_PID, LOG_USER);

	return 0;
}

int main(int argc, char **argv){
	using namespace::std;
	string	root_dir;

	opt_read(argc, argv, root_dir);
	daemon_init(root_dir.c_str());
	Config conf(root_dir.c_str());

	dir_init();

	Network Network(conf.get_server_port(), conf.get_root_dir());
	Request	req;
	Response rsp;
	Fastcgi fcgi(conf);

	Network.request_process(req, rsp, conf, fcgi);

	return 0;
}
