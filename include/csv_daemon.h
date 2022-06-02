#ifndef __CSV_DAEMON_H__
#define __CSV_DAEMON_H__

#define NAME_DAEMON_SERVER		"'daemon_server'"
#define NAME_DAEMON_CLIENT		"'daemon_client'"

#define PORT_DAEMON_UDP			(4353)
#define DAEMON_BEACON			(0x43534353)	// CSCS

#define DAEMON_TIMEO			(10)			// 10s

#define MAX_LEN_CMDLINE			(128)

struct csv_daemon_t {
	char					cmdline[MAX_LEN_CMDLINE];	// 程序启动命令
	char					*name_server;
	char					*name_client;
	int						fd_server;			// 子进程拥有
	int						fd_client;			// 父进程拥有

	int						port;				// udp 端口
	uint32_t				beacon;				// 信标数据
	pid_t					pid;				// 进程id

	struct sockaddr_in		local_addr;			// 子进程使用
	struct sockaddr_in		target_addr;		// 父进程使用
};

extern int csv_daemon_client_feed (struct csv_daemon_t *pDAEMON);

extern int csv_daemon_init (int argc, char **argv);


#endif  /* __CSV_DAEMON_H__ */



