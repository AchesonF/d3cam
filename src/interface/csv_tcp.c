#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif



int csv_tcp_reading_trigger (struct csv_tcp_t *pTCP)
{
	int nRead = 0;
#define MAX_LEN_FRAME 4096
	uint8_t rbuf[MAX_LEN_FRAME] = {0};

	nRead = pTCP->recv(rbuf, MAX_LEN_FRAME);
	if (nRead < 0) {
		log_err("%s : recv %d", pTCP->name, nRead);
		return -1;
	} else if (nRead == 0) {
		log_info("%s : EOF", pTCP->name);
		pTCP->close();
	} else {
		// todo queue msg

	}

	return 0;
}

int csv_tcp_init (void)
{
	int ret = 0;

	ret = csv_tcp_local_init();
	ret |= csv_tcp_remote_init();

	return 0;
}

int csv_tcp_deinit (void)
{
	int ret = 0;
	ret = csv_tcp_local_deinit();
	ret != csv_tcp_remote_deinit();

	return 0;
}

#ifdef __cplusplus
}
#endif


