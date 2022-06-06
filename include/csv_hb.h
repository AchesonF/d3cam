#ifndef __CSV_HB_H__
#define __CSV_HB_H__

#ifdef __cplusplus
extern "C" {
#endif

#define HEARTBEAT_BEACON			"CSHB"

#define HEARTBEAT_TIMEO				(5)			// 10s

#define MAX_LEN_STARTCMD			(256)

struct csv_hb_t {

	char			cmdline[MAX_LEN_STARTCMD];	// 程序启动命令
	int				pipefd[2];	// 0:read 1:write
	char			*beacon;		// 信标数据

};

extern int csv_hb_write (void);

extern int csv_hb_init (int argc, char **argv);



#ifdef __cplusplus
}
#endif

#endif /* __CSV_HB_H__ */

