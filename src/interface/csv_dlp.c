#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif


static uint8_t dlp_ctrl_cmd[TOTAL_DLP_CMD][LEN_DLP_CTRL] = {
	{0x36, 0xAA, 0x01, 0x28, 0xFF, 0x03, 0xFF, 0x32, 0x00, 0x00, 0xAA, 0xAA},	// PINSTRIPE
	{0x36, 0xAA, 0x02, 0x28, 0xFF, 0x03, 0xFF, 0x32, 0x00, 0x00, 0xAA, 0xAA},	// DEMARCATE
	{0x36, 0xAA, 0x03, 0x28, 0xFF, 0x03, 0xFF, 0x32, 0x00, 0x00, 0xAA, 0xAA},	// WIDISTRIPE
	{0x36, 0xAA, 0x04, 0x28, 0xFF, 0x03, 0xFF, 0x32, 0x00, 0x00, 0xAA, 0xAA},	// FOCUS
	{0x36, 0xAA, 0x05, 0x28, 0xFF, 0x03, 0xFF, 0x32, 0x00, 0x00, 0xAA, 0xAA},	// BRIGHT
	{0x36, 0xAA, 0x06, 0x28, 0xFF, 0x03, 0xFF, 0x32, 0x00, 0x00, 0xAA, 0xAA},	// SINGLE_SINE
	{0x36, 0xAA, 0x07, 0x28, 0xFF, 0x03, 0xFF, 0x32, 0x00, 0x00, 0xAA, 0xAA},	// SINGLE_WIDESTRIPE_SINE
	{0x36, 0xAA, 0x21, 0x04, 0xFF, 0x03, 0xFF, 0x32, 0x00, 0x00, 0xAA, 0xAA},	// FOCUS_BRIGHT
	{0x36, 0xAA, 0x21, 0x05, 0xFF, 0x03, 0xFF, 0x32, 0x00, 0x00, 0xAA, 0xAA},	// LIGHT_BRIGHT
	{0x36, 0xAA, 0x21, 0x06, 0xFF, 0x03, 0xFF, 0x32, 0x00, 0x00, 0xAA, 0xAA},	// PINSTRIPE_SINE_BRIGHT
	{0x36, 0xAA, 0x21, 0x07, 0xFF, 0x03, 0xFF, 0x32, 0x00, 0x00, 0xAA, 0xAA}	// WIDESTRIPE_SINE_BRIGHT
};

int csv_dlp_write (uint8_t idx)
{
	int ret = 0;
	struct csv_dlp_t *pDLP = &gCSV->dlp;

	if (pDLP->fd <= 0) {
		return -1;
	}

	ret = csv_tty_write(pDLP->fd, dlp_ctrl_cmd[idx], sizeof(dlp_ctrl_cmd[idx]));
	if (ret < 0) {
		log_err("ERROR : %s write failed.", pDLP->name);
		return -1;
	}

	log_hex(STREAM_TTY, dlp_ctrl_cmd[idx], ret, "DLP write");

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


