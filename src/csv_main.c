#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MIN(a,b) (((a)>(b))?(b):(a))
#define MAX(a,b) (((a)>(b))?(a):(b))

struct csv_info_t *gCSV = NULL;

struct csv_product_t gPdct = {
	.tlog = 0,
	.tdata = 0,

    .app_name = CONFIG_APP_NAME,
    .app_version = SOFTWARE_VERSION,
    .app_buildtime = SOFT_BUILDTIME,
    .compiler_version = COMPILER_VERSION,

    .uptime = 0,
    .app_runtime = 0,
    .fd_lock = -1,
};

static void csv_trace (int signum)
{
	void *array[256];
	size_t size;
	char **strings;
	int i;
	FILE *fp;
	char *strSIG = NULL;

	switch (signum) {
	case SIGABRT:
		strSIG = toSTR(SIGABRT);
		break;
	case SIGSEGV:
		strSIG = toSTR(SIGSEGV);
		break;
	default:
		strSIG = "htop maybe help";
		break;
	}


	signal(signum, SIG_DFL);
	size = backtrace(array, 256);
	strings = (char **)backtrace_symbols(array, size);

	fp = fopen(FILE_PATH_BACKTRACE, "a+");

	fprintf(fp, "==== received signum[%d]=%s backtrace ====\n", signum, strSIG);
	fprintf(stderr, "==== received signum[%d]=%s backtrace ====\n", signum, strSIG);
	fprintf(fp, "%s\n", gPdct.app_info);
	fprintf(stderr, "%s\n", gPdct.app_info);
	for (i = 0; i < size; i++) {
		fprintf(fp, "%d %s \n", i, strings[i]);	
		fprintf(stderr, "%d %s \n", i, strings[i]);	
	}

	free(strings);
	fclose(fp);

	close(gPdct.fd_lock);

	sync();

	//system("reboot");
	_exit(1);
}

void csv_stop (int signum)
{
	int ret = 0;
	char *strSIG = NULL;

	switch (signum) {
	case SIGINT:
		strSIG = toSTR(SIGINT);
		break;
	case SIGSTOP:
		strSIG = toSTR(SIGSTOP);
		break;
	default:
		strSIG = "htop maybe help";
		break;
	}

	csv_gvcp_deinit();

	csv_dlp_deinit();

	csv_msg_deinit();

	csv_tcp_deinit();

	csv_uevent_deinit();

	csv_mvs_deinit();

	csv_web_deinit();

	csv_tick_deinit();


	if (gCSV != NULL) {
		free(gCSV);
	}

	log_info("OK : Stop process pid[%d] via signum[%d]=%s", 
		getpid(), signum, strSIG);

	close(gPdct.fd_lock);

	sync();

	exit(ret);
}

static struct csv_info_t *csv_global_init (void)
{
	struct csv_info_t *pCSV = NULL;

	pCSV = (struct csv_info_t *) malloc(sizeof(*pCSV));
	if (pCSV == NULL) {
		log_alert("ERROR : malloc csv_info_t");
		exit(-1);
		return NULL;
	}

	memset(pCSV, 0, sizeof(*pCSV));

	return pCSV;
}

static int csv_lock_pid (void)
{
	int fd = -1, ret = 0;
	char strpid[32] = {0};

	fd = open(FILE_PID_LOCK, O_RDWR|O_CREAT, 0600);
	if (fd < 0) {
		log_err("ERROR : open '%s'", FILE_PID_LOCK);
		log_warn("You should delete '%s' first.", FILE_PID_LOCK);

		exit(EXIT_FAILURE);
	}

	ret = lockf(fd, F_TLOCK, 0);
	if (ret < 0) {
		log_err("ERROR : lockf fd(%d)", fd);
		log_warn("You don't need to start me again!");

		exit(EXIT_FAILURE);
	}

	gPdct.fd_lock = fd;

	snprintf(strpid, 32, "%d", getpid());
	ret = write(fd, strpid, strlen(strpid));
	if (ret < 0) {
		log_err("ERROR : write fd(%d)", fd);
	}

	log_info("My pid id %d.", getpid());
}

static void print_usage (const char *prog)
{
	printf("\nUsage: %s [-dmhvt] [-D opt]\n", prog);
	puts("  -d --debug    Debug mode.\n"
		"  -D --Data     Show data flow. opt: \n\t\t1:tcp 2:tty "\
						"3:udp 4:sql ...9: data 255:all\n"
		"  -m --daemon   Disable daemon.\n"
		"  -h --help     Show this info.\n"
		"  -v --version  Build version.\n"
		"  -W --WebCfg   Web config file, suffix as '.conf'\n"
		"  -t --work     System working hour.\n");
}

static const struct option lopts[] = {
	{ "debug",		no_argument,	0, 'd' },
	{ "Data",	required_argument,	0, 'D' },
	{ "help",		no_argument,	0, 'h' },
	{ "daemon",		no_argument,	0, 'm' },
	{ "version",	no_argument,	0, 'v' },
	{ "work",		no_argument,	0, 't' },
	{ "WebCfg",	required_argument,	0, 'W' },
	{ NULL,			0,				0,	0 },
};

static void startup_opts (int argc, char **argv)
{
	int c = 0;
	struct csv_product_t *pPdct = &gPdct;

	printf("\n");

	utility_conv_buildtime();
	utility_conv_kbuildtime();
	csv_life_init();

	while (1) {
		c = getopt_long(argc, argv, "W:D:dmhvt", lopts, NULL);
		if (c == -1) {
			break;
		}

		switch (c) {
		case 'd':
			pPdct->tlog = true;
			break;
		case 'm':
			pPdct->dis_daemon = true;
			break;

		case 'h':
		case '?':
			print_usage(argv[0]);
			exit(1);
			break;
		case 'D':
			pPdct->tdata = (uint8_t)atoi(optarg);
			break;
		case 'W':
			memset(pPdct->WebCfg, 0, 256);
			strncpy(pPdct->WebCfg, optarg, 256);
			break;
		case 'v':
			printf("Version : %s\n", pPdct->app_info);
			exit(1);
			break;
		case 't':
		{
			struct csv_lifetime_t *pLIFE = &pPdct->lifetime;
			printf("working hour : \n\ttotal: %u mins\n\tcurrent: %u mins\n\tlast: %u mins\n\tcount: %u\n",
				pLIFE->total, pLIFE->current, pLIFE->last, pLIFE->count);

			exit(1);
		}
		break;

		default:
			print_usage(argv[0]);
			exit(1);
			break;
		}
	}

	csv_lock_pid();

	utility_calibrate_clock();

	log_info("%s via GCC/GXX %s", pPdct->app_info, pPdct->compiler_version);
	log_info("%s (%s)", pPdct->kernel_version, pPdct->kernel_buildtime);

	if (!pPdct->dis_daemon) {
		csv_hb_init(argc, argv);
	}
}

int csv_init (struct csv_info_t *pCSV)
{
	csv_file_init();

	csv_json_init();

	csv_eth_init();

	csv_dlp_init();

	csv_gvcp_init();

	csv_tcp_init();

	csv_msg_init();

	csv_uevent_init();

	csv_mvs_init();

	csv_stat_init();

	csv_web_init();

	csv_tick_init();

	csv_life_start();

	signal(SIGSEGV, csv_trace);		// 11
	signal(SIGABRT, csv_trace);		// 6

	signal(SIGINT, csv_stop);
	signal(SIGSTOP, csv_stop);

	signal(SIGPIPE, SIG_IGN);

	return 0;
}

int main (int argc, char **argv)
{
	int ret = 0, i = 0;
	int maxfd = 0;
	struct timeval tv;
	fd_set readset, writeset;

	startup_opts(argc, argv);

	gCSV = csv_global_init();

	csv_init(gCSV);

	struct csv_uevent_t *pUE = &gCSV->uevent;
	struct csv_gvcp_t *pGVCP = &gCSV->gvcp;
	struct csv_tick_t *pTICK = &gCSV->tick;
	struct csv_tcp_t *pTCPL = &gCSV->tcpl;

	while (1) {
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		FD_ZERO(&readset);
		FD_ZERO(&writeset);

		if (pGVCP->fd > 0) {
			maxfd = MAX(maxfd, pGVCP->fd);
			FD_SET(pGVCP->fd, &readset);
		}

		if (pUE->fd > 0) {
			maxfd = MAX(maxfd, pUE->fd);
			FD_SET(pUE->fd, &readset);
		}

		if (pTCPL->fd_listen > 0) {
			maxfd = MAX(maxfd, pTCPL->fd_listen);
			FD_SET(pTCPL->fd_listen, &readset);
		}

		if ((pTCPL->accepted)&&(pTCPL->fd > 0)) {
			maxfd = MAX(maxfd, pTCPL->fd);
			FD_SET(pTCPL->fd, &readset);

			if (pTCPL->beat.timerfd > 0) {
				maxfd = MAX(maxfd, pTCPL->beat.timerfd);
				FD_SET(pTCPL->beat.timerfd, &readset);
			}
		}

		if (pTICK->fd > 0) {
			maxfd = MAX(maxfd, pTICK->fd);
			FD_SET(pTICK->fd, &readset);
		}

		ret = select(maxfd+1, &readset, &writeset, NULL, &tv);
		switch (ret) {
		case 0:			// timeout
			continue;
		break;
		case -1:		// error must be restart ?
			log_err("ERROR : select");
		break;
		default:		// number of descriptors
			// log_debug("select %d", ret);
		break;
		}

		if ((pGVCP->fd > 0)&&(FD_ISSET(pGVCP->fd, &readset))) {
			csv_gvcp_trigger(pGVCP);
		}

		if ((pUE->fd > 0)&&(FD_ISSET(pUE->fd, &readset))) {
			csv_uevent_trigger(pUE);
		}

		if ((pTCPL->fd_listen > 0)&&(FD_ISSET(pTCPL->fd_listen, &readset))) {
			if (pTCPL->accepted) {
				csv_tcp_local_close();
			}
			csv_tcp_local_accept();
		}

		if ((pTCPL->accepted)&&(FD_ISSET(pTCPL->fd, &readset))) {
			csv_tcp_reading_trigger(pTCPL);
		}

		if ((pTCPL->beat.timerfd > 0)&&(FD_ISSET(pTCPL->beat.timerfd, &readset))) {
			csv_beat_timer_trigger(&pTCPL->beat);
		}

		if ((pTICK->fd > 0)&&(FD_ISSET(pTICK->fd, &readset))) {
			csv_tick_timer_trigger(pTICK);
		}

	}



	return 0;
}


#ifdef __cplusplus
}
#endif




