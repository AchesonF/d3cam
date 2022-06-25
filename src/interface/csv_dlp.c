#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

static uint8_t rl_cmd_data[RL_CMD_NUM][20] = {
	{0x11, 0x11, 0x11, 0x11, 0x22, 0x53, 0x00, 0x0c, 
		0x00, 0xff, 0x02, 0x03, 0x00, 0x00, 0x00, 0xaa, 
		0x60, 0x0b, 0x0d, 0x0a},						// OPEN_LOGO_LED
    {0x11, 0x11, 0x11, 0x11, 0x22, 0x53, 0x00, 0x0c, 
    	0x00, 0xff, 0x02, 0x03, 0x00, 0x00, 0x00, 0x55, 
    	0x7E, 0xfb, 0x0d, 0x0a},						// CLOSE_LOGO_LED
    {0x11, 0x11, 0x11, 0x11, 0x22, 0x53, 0x00, 0x0c, 
    	0x00, 0xff, 0x02, 0x02, 0x00, 0x00, 0x00, 0xaa, 
    	0xca, 0x5a, 0x0d, 0x0a},						// OPEN_LASER_LED
    {0x11, 0x11, 0x11, 0x11, 0x22, 0x53, 0x00, 0x0c, 
    	0x00, 0xff, 0x02, 0x02, 0x00, 0x00, 0x00, 0x55, 
    	0xd4, 0xaa, 0x0d, 0x0a},						// CLOSE_LASER_LED
    {0x11, 0x11, 0x11, 0x11, 0x22, 0x53, 0x00, 0x0c, 
    	0x00, 0xff, 0x02, 0x01, 0x00, 0x00, 0x00, 0xaa, 
    	0x24, 0x88, 0x0d, 0x0a},						// OPEN_SYNC
    {0x11, 0x11, 0x11, 0x11, 0x22, 0x53, 0x00, 0x0c, 
    	0x00, 0xff, 0x02, 0x01, 0x00, 0x00, 0x00, 0x55, 
    	0x3a, 0x78, 0x0d, 0x0a},						// CLOSE_SYNC
    {0x11, 0x11, 0x11, 0x11, 0x22, 0x53, 0x00, 0x0c, 
    	0x00, 0xff, 0x02, 0x05, 0x00, 0x00, 0x00, 0xaa, 
    	0x28, 0x97, 0x0d, 0x0a},						// OPEN_FAN
    {0x11, 0x11, 0x11, 0x11, 0x22, 0x53, 0x00, 0x0c, 
    	0x00, 0xff, 0x02, 0x05, 0x00, 0x00, 0x00, 0x55, 
    	0x36, 0x67, 0x0d, 0x0a}							// CLOSE_FAN
};


int csv_dlp_write (uint8_t idx)
{
	int ret = 0;
	struct csv_dlp_t *pDLP = &gCSV->dlp;

	if (pDLP->fd <= 0) {
		return -1;
	}

	ret = csv_tty_write(pDLP->fd, rl_cmd_data[idx], sizeof(rl_cmd_data[idx]));
	if (ret < 0) {
		log_err("ERROR : %s write failed.", pDLP->name);
		return -1;
	}

	log_hex(STREAM_TTY, rl_cmd_data[idx], ret, "DLP write");

	return ret;
}

int csv_dlp_read (void)
{
	int ret = 0;
	struct csv_dlp_t *pDLP = &gCSV->dlp;

	if (pDLP->fd <= 0) {
		return -1;
	}

	ret = csv_tty_read(pDLP->fd, pDLP->rbuf, SIZE_DLP_BUFF);
	if (ret < 0) {
		log_err("ERROR : %s read failed.", pDLP->name);
		return -1;
	}

	pDLP->rlen = ret;

	log_hex(STREAM_TTY, pDLP->rbuf, ret, "DLP read");

	return ret;
}


int csv_dlp_early_config (void)
{
	int ret = 0;

	ret = csv_dlp_write(OPEN_FAN);

	// some more


	return ret;
}






int csv_dlp_init (void)
{
	int ret = 0;
	struct csv_dlp_t *pDLP = &gCSV->dlp;
	struct csv_tty_param_t *pParam = &pDLP->param;


	pDLP->dev = DEV_TTY_DLP;
	pDLP->name = NAME_DEV_DLP;
	pDLP->fd = -1;

	pParam->baudrate = DEFAULT_DLP_BAUDRATE;
	pParam->databits = 8;
	pParam->stopbits = 1;
	pParam->parity = 'n';
	pParam->flowcontrol = 0;
	pParam->delay = 1;

	ret = csv_tty_init(pDLP->dev, pParam);
	if (ret <= 0) {
		log_info("ERROR : init %s.", pDLP->name);
		return -1;
	}

	pDLP->fd = ret;

	log_info("OK : init %s as fd(%d).", pDLP->name, pDLP->fd);

	return 0;
}


int csv_dlp_deinit (void)
{
	struct csv_dlp_t *pDLP = &gCSV->dlp;

	if (pDLP->fd > 0) {
		csv_dlp_write(CLOSE_FAN);


		if (close(pDLP->fd) < 0) {
			log_err("ERROR : close %s : fd(%d) failed.", pDLP->name, pDLP->fd);
			return -1;
		}

		pDLP->fd = -1;
		log_info("OK : close %s : fd(%d).", pDLP->name, pDLP->fd);
	}

	return 0;
}


#ifdef __cplusplus
}
#endif


