#include "inc_files.h"


#ifdef __cplusplus
extern "C" {
#endif

static int csv_gpi_open (struct csv_gpi_t *pGPI)
{
	int fd = -1;

    fd = open(pGPI->dev, O_RDONLY);
    if (fd < 0) {
        log_err("ERROR : open '%s'.", pGPI->dev);
        return -1;
    }

    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        log_err("ERROR : fcntl %s.", pGPI->name);
        return -1;
    }

    pGPI->fd = fd;
    log_info("OK : open %s : '%s' as fd(%d).", pGPI->name, pGPI->dev, fd);

    return 0;
}

static int csv_gpi_close (struct csv_gpi_t *pGPI)
{
	if (pGPI->fd > 0) {
		if (close(pGPI->fd) < 0) {
			log_err("ERROR : close %s.", pGPI->name);
			return -1;
		}

		pGPI->fd = -1;
		log_info("OK : close %s.", pGPI->name);
	}

	return 0;
}

int csv_gpi_trigger (struct csv_gpi_t *pGPI)
{
	if (read(pGPI->fd, &pGPI->key, sizeof(pGPI->key)) == sizeof(pGPI->key)) {
		if (EV_KEY == pGPI->key.type) {
			pGPI->code = pGPI->key.code;
			pGPI->value = pGPI->key.value;
			log_debug("key 0x%X : %d", pGPI->code, pGPI->value);

			switch (pGPI->code) {
			case GPI_IN_FORCE:
				if (GPI_LOW == pGPI->value) {
					pGPI->status[IDX_IN_1] = GPI_LOW;
#if defined(USE_HK_CAMS)
					gCSV->mvs.grab_type = MVS_CALIBRATE;
					pthread_cond_broadcast(&gCSV->mvs.cond_grab);	// 下降沿触发
#endif
				} else if (GPI_HIGH == pGPI->value) {
					pGPI->status[IDX_IN_1] = GPI_HIGH;
				}
				break;
			case GPI_IN_POWER:
				if (GPI_LOW == pGPI->value) {
					// todo
					pGPI->status[IDX_IN_2] = GPI_LOW;
#if defined(USE_HK_CAMS)
					gCSV->mvs.grab_type = MVS_POINTCLOUD;
					pthread_cond_broadcast(&gCSV->mvs.cond_grab);	// 下降沿触发
#endif
				} else if (GPI_HIGH == pGPI->value) {
					// todo
					pGPI->status[IDX_IN_2] = GPI_HIGH;
				}
				break;
			default:
				//log_debug("Not support key");
				break;
			}
		}
	}

	return 0;
}

int csv_gpi_init (void)
{
	struct csv_gpi_t *pGPI = &gCSV->gpi;

	pGPI->name = NAME_INPUT;
	pGPI->dev = DEV_GPIO_INPUT;
	pGPI->fd = -1;
	pGPI->status[IDX_IN_1] = GPI_LOW;
	pGPI->status[IDX_IN_2] = GPI_LOW;

	return csv_gpi_open(pGPI);
}

int csv_gpi_deinit (void)
{
	return csv_gpi_close(&gCSV->gpi);
}

#ifdef __cplusplus
}
#endif



