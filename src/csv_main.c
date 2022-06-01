#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

struct csv_info_t *gCSV = NULL;

struct csv_product_t gPdct = {
	.tlog = 0,
	.tdata = 0,

    .app_name = CONFIG_APP_NAME,
    .app_version = SOFTWARE_VERSION,
    .app_buildtime = SOFT_BUILDTIME,

    .uptime = 0,
    .app_runtime = 0,
};

static void csv_trace (int signum)
{
	void *array[100];
	size_t size;
	char **strings;
	int i;
	FILE *fp;

	signal(signum, SIG_DFL);
	size = backtrace(array, 100);
	strings = (char **)backtrace_symbols(array, size);

	fp = fopen(FILE_PATH_BACKTRACE, "a+");

	fprintf(fp, "==== received signum[%d] backtrace ====\n", signum);
	fprintf(stderr, "==== received signum[%d] backtrace ====\n", signum);
	for (i = 0; i < size; i++) {
		fprintf(fp, "%d %s \n", i, strings[i]);	
		fprintf(stderr, "%d %s \n", i, strings[i]);	
	}

	free(strings);
	fclose(fp);

	sync();

	//system("reboot");
	_exit(1);
}

void csv_stop (int signum)
{
	int ret = 0;

	if (gCSV != NULL) {
		free(gCSV);
	}
printf("OK : Stop process pid[%d] via signum[%d]\n", 
		getpid(), signum);
//	log_info(LOG_FMT"OK : Stop process pid[%d] via signum[%d]", 
//		LOG_ARGS, getpid(), signum);

	sync();

	if (signum == SIGPWR) {
		ret = system("reboot");
		sleep(1);
	}

	exit(ret);
}

static struct csv_info_t *csv_global_init (void)
{
	struct csv_info_t *pCSV = NULL;

	pCSV = (struct csv_info_t *) malloc(sizeof(*pCSV));
	if (pCSV == NULL) {
//		log_exit(LOG_FMT"ERROR : malloc csv_info_t", LOG_ARGS);

		return NULL;
	}

	memset(pCSV, 0, sizeof(*pCSV));

	return pCSV;
}

static void print_usage (const char *prog)
{
	printf("\nUsage: %s [-dhvt] [-D opt]\n", prog);
	puts("  -d --debug    Debug mode.\n"
		"  -D --Data     Show data flow. opt: \n\t\t1:tcp 2:tty 3:udp 4:sql ...255:all\n"
		"  -h --help     Show this info.\n"
		"  -v --version  Build version.\n"
		"  -t --work     System working hour.\n");
}

static const struct option lopts[] = {
	{ "debug",		no_argument,	0, 'd' },
	{ "Data",	required_argument,	0, 'D' },
	{ "help",		no_argument,	0, 'h' },
	{ "version",	no_argument,	0, 'v' },
	{ "work",		no_argument,	0, 't' },
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
		c = getopt_long(argc, argv, "D:dhvt", lopts, NULL);
		if (c == -1) {
			break;
		}

		switch (c) {
		case 'd':
			pPdct->tlog = true;
		break;

		case 'h':
		case '?':
			print_usage(argv[0]);
			exit(1);
		break;
		case 'D':
			pPdct->tdata = (uint8_t)atoi(optarg);
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

	utility_calibrate_clock();

//	log_info("%s", pPdct->app_info);
//	log_info("%s (%s)", pPdct->kernel_version, pPdct->kernel_buildtime);

//	csv_daemon_init();
}

int csv_init (struct csv_info_t *pCSV)
{
	csv_file_init();

	csv_tick_init();


	csv_life_start();

	signal(SIGSEGV, csv_trace);
	signal(SIGABRT, csv_trace);

	signal(SIGINT, csv_stop);
	signal(SIGSTOP, csv_stop);

	signal(SIGPIPE, SIG_IGN);

	return 0;
}

int main (int argc, char **argv)
{
	int ret = 0;
	int maxfd = 0;
	struct timeval tv;
	fd_set readset, writeset;

	startup_opts(argc, argv);

	gCSV = csv_global_init();

	csv_init(gCSV);

	struct csv_tick_t *pTICK = &gCSV->tick;


	while (1) {
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		FD_ZERO(&readset);
		FD_ZERO(&writeset);

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
//			log_err(LOG_FMT"ERROR : select", LOG_ARGS);
		break;
		default:		// number of descriptors
			// log_dbg(LOG_FMT"select %d", LOG_ARGS, ret);
		break;
		}


		if (FD_ISSET(pTICK->fd, &readset)) {
			csv_tick_timer_trigger(pTICK);
		}


	}



	return 0;
}


#ifdef __cplusplus
}
#endif




