#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

static int csv_hb_read (struct csv_hb_t *pHB)
{
	int nRet = 0;
	char rBuf[16] = {0};

	if (pHB->pipefd[0] < 0) {
		return -1;
	}

	nRet = read(pHB->pipefd[0], rBuf, 16);
	if (nRet > 0) {
//		log_debug("R: %d : %s vs %s", nRet, rBuf, gPdct.hb.beacon);
		if (strncmp(rBuf, pHB->beacon, strlen(pHB->beacon)) == 0) {
			return nRet;
		}
	}

	return nRet; // =0:EOF <0: ERROR
}

int csv_hb_write (void)
{
	int nRet = 0;
	struct csv_hb_t *pHB = &gPdct.hb;

	if (pHB->pipefd[1] <= 0) {
		return -1;
	}

//	log_debug("S: %s", pHB->beacon);
	nRet = write(pHB->pipefd[1], pHB->beacon, strlen(pHB->beacon));
	if (nRet > 0) {
		return 0;
	}

	return -1;
}

static int csv_hb_startcmd_get (struct csv_hb_t *pHB, int argc, char **argv)
{
	int i = 0;
	char str_opt[MAX_LEN_STARTCMD] = {0};
	memset(pHB->cmdline, 0, MAX_LEN_STARTCMD);

	for (i = 0; i < argc; i++) {
		sprintf(str_opt, "%s ", argv[i]);
		strcat(pHB->cmdline, str_opt);
	}

	strcat(pHB->cmdline, "&");

	log_info("OK : get cmdline : '%s'", pHB->cmdline);

	return 0;
}

static int csv_hb_server_init (struct csv_hb_t *pHB)
{
	int ret = 0, timeo = 0;
	struct timeval tv;
	fd_set readset;
	pid_t ppid = getppid();
	int fd = pHB->pipefd[0];

    ret = fcntl(fd, F_SETFL, O_NONBLOCK);
    if (ret < 0) {
        log_err("ERROR : fcntl");
        close(fd);
        exit(EXIT_FAILURE);
    }

	log_info("OK : pid(%d) fork from ppid(%d).", getpid(), ppid);

	while (1) {
		tv.tv_sec = 2;
		tv.tv_usec = 0;

		FD_ZERO(&readset);

		if (fd > 0) {
			FD_SET(fd, &readset);
		}

		ret = select(fd + 1, &readset, NULL, NULL, &tv);
		if (ret < 0) {
			log_err("ERROR : child select (%d).", ret);
			exit(EXIT_FAILURE);
		} else if (ret == 0) {	// timeout
			if (++timeo >= HEARTBEAT_TIMEO) {
				close(fd);
				fd = -1;

				log_info("ERROR : lost parent process.");
				kill(ppid, SIGKILL);

				if (isprint(pHB->cmdline[0])) {
					ret = system(pHB->cmdline);
					log_info("OK : re-launch '%s' %d.", pHB->cmdline, ret);
				}

				log_info("OK : pid(%d) says 'byebye'.", getpid());
				exit(ret);
			}
		}

		if (FD_ISSET(fd, &readset)){
			ret = csv_hb_read(pHB);
			if (ret > 0) {
				timeo = 0;
			} else if (ret <= 0) {
				close(fd);
				fd = -1;

				log_info("ERROR : parent process pipe broken.");
				kill(ppid, SIGKILL);
				if (isprint(pHB->cmdline[0])) {
					ret = system(pHB->cmdline);
					log_info("OK : re-launch '%s' %d.", pHB->cmdline, ret);
				}

				log_info("OK : pid(%d) says 'byebye'.", getpid());

				exit(ret);
			}
		}
	}

	return -1;
}

int csv_hb_init (int argc, char **argv)
{
	int ret = 0;
	struct csv_hb_t *pHB = &gPdct.hb;
	pid_t cpid = 0;

	pHB->beacon = HEARTBEAT_BEACON;
	csv_hb_startcmd_get(pHB, argc, argv);

	ret = pipe(pHB->pipefd);
	if (ret < 0) {
		log_err("ERROR : pipe");
		return -1;
	}

	log_info("pipe %d  %d\n", pHB->pipefd[0], pHB->pipefd[1]);

	cpid = fork();

	switch (cpid) {
	case 0:		// child
		close(pHB->pipefd[1]);	// not use in child process
		csv_hb_server_init(pHB);
		log_info("ERROR : heartbeat die. ~~~~");
		exit(EXIT_FAILURE);
		break;
	case -1:	// error
		log_err("ERROR : fork");
		return -1;
		break;
	default:	// parent
		close(pHB->pipefd[0]);	// not use in parent process
		log_info("OK : pid(%d) forked a child pid(%d).", getpid(), cpid);
		break;
	}

	return 0;
}

#ifdef __cplusplus
}
#endif


