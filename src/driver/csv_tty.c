#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

const uint32_t TableBaudrate[TOTAL_BAUDRATE] = {
	4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600};

const char TableParity[TOTAL_PARITY] = {'n', 'o', 'e'};


static int csv_tty_config (int fd, int speed, int databits, int stopbits, int parity, int delay)
{
	struct termios tio;
	if (tcgetattr(fd, &tio) != 0) {
		log_err("ERROR : tcgetattr");
		return -1;
	}
	
	/* 波特率 */
	int baudrate;
	switch (speed) {	
	case 1200:
		baudrate=B1200;
		break;
	case 1800:
		baudrate=B1800;
		break;
	case 2400:
		baudrate=B2400;
		break;
	case 4800:
		baudrate=B4800;
		break;
	case 9600:
		baudrate=B9600;
		break;
	case 19200:
		baudrate=B19200;
		break;
	case 38400:
		baudrate=B38400;
		break;
	case 57600:
		baudrate=B57600;
		break;
	case 115200:
		baudrate=B115200;
		break;
	case 230400:
		baudrate=B230400;
		break;
	case 460800:
		baudrate=B460800;
		break;
	case 576000:
		baudrate=B576000;
		break;
	case 921600:
		baudrate=B921600;
		break;	
	case 2000000:
		baudrate=B2000000;
		break;
	default:		
		log_info("ERROR : invalid baudrate(%d)", speed);
		return -1;
	}
	
	if (-1 == cfsetispeed(&tio, baudrate)) {
		log_err("ERROR : cfsetispeed");
		return -1;
	}
	
	if (-1 == cfsetospeed(&tio, baudrate)) {
		log_err("ERROR : cfsetispeed");
		return -1;
	}
	
	/* 数据长度 */
	tio.c_cflag &= ~CSIZE;
	switch (databits) {
	case 7:
		tio.c_cflag |= CS7;
		break;
	case 8:
		tio.c_cflag |= CS8;
		break;
	default:		
		log_info("ERROR : invalid databits(%d)", databits);
		return -1;
	}
	
	/* 奇偶校验 */
	switch (parity) {
	case 'n':
	case 'N':
		tio.c_cflag &= ~PARENB;
		break;
	case 'o':
	case 'O':
		tio.c_cflag |= (PARODD | PARENB);
		break;
	case 'e':
	case 'E':
		tio.c_cflag |= PARENB;
		tio.c_cflag &= ~PARODD;		
		break;
	default:		
		log_info("ERROR : invalid parity(%c)", parity);
		return -1;
	}
	
	/* 停止位 */
	switch (stopbits) {
	case 1:
		tio.c_cflag &= ~CSTOPB;
		break;
	case 2:
		tio.c_cflag |= CSTOPB;
		break;
	default:		
		log_info("ERROR : invalid stopbits(%d)", stopbits);
		return -1;
	}

	tio.c_iflag = IGNPAR;
	tio.c_oflag = 0;
 	tio.c_lflag = 0;
#if 0
	tio.c_cc[VTIME] = 0;	/* NOTE: 如果设为10则会阻塞1秒 */
	tio.c_cc[VMIN] = 1;
#else
	tio.c_cc[VTIME] = delay;
	tio.c_cc[VMIN] = 0;	
#endif

	tcflush(fd, TCIFLUSH);
	if (tcsetattr(fd, TCSANOW, &tio) != 0) {
		log_err("ERROR : tcsetattr");
		return -1;
	}

	return 0;
}

// 清空输入和输出数据
int csv_tty_flush (int fd)
{
	return tcflush(fd, TCIOFLUSH);
}

// 等待所有输出都被传输
int csv_tty_drain (int fd)
{
	return tcdrain(fd);
}

/*
 * >0	读到的字节数(EAGAIN)	
 * 0	EOF
 * -1	其他错误
 * -2	EAGAIN
 */
int csv_tty_read (int fd, uint8_t *buf, int nbytes)
{
	ssize_t ret = 0;
	uint32_t n_read = 0;

	while (n_read < nbytes) {
		ret = read(fd, buf+n_read, nbytes-n_read);
		if (ret <= 0)
			break;
		n_read += ret;
	}

	if (ret == 0) {	/* EOF */
		return n_read;
	} else if (ret < 0) {
		log_err("ERROR : read fd(%d)", fd);
		if (errno == EAGAIN) {			
			return n_read;
		}
		return -1;
	}

	return n_read;
}

int csv_tty_write (int fd, uint8_t *buf, int nbytes)
{
	ssize_t ret;
	uint32_t n_written = 0;

	while (n_written < nbytes) {
		ret = write(fd, buf+n_written, nbytes-n_written);
		if (ret < 0) {
			if (errno == EAGAIN)
				return n_written;
			return -1;
		}
		
		n_written += ret;
	}

	return n_written;
}

int csv_tty_init (char *tty_name, struct csv_tty_param_t *pPARAM)
{
	int ret = 0;

	int fd = open(tty_name, O_RDWR | O_NOCTTY);
	if (fd < 0) {
		log_err("ERROR : open");
		return fd;
	}

	ret = csv_tty_config(fd, pPARAM->baudrate, pPARAM->databits, 
		pPARAM->stopbits, pPARAM->parity, pPARAM->delay);
	if (ret < 0) {
		close(fd);
		return -1;
	}

	log_info("OK : config '%s' via '%d,%c,%d,%d,%d'", tty_name, pPARAM->baudrate, 
		pPARAM->parity, pPARAM->databits, pPARAM->stopbits, pPARAM->delay);

	return fd;
}

int csv_tty_deinit (int fd)
{
	if (fd > 0) {
		if (close(fd) < 0) {
			log_err("ERROR : close tty fd(fd).", fd);
			return -1;
		}

		log_info("OK : close fd(%d).", fd);
	}

	return 0;
}

#ifdef __cplusplus
}
#endif


