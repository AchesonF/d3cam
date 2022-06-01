#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

// save lifetime to file
static int file_lifetime_write (void)
{
	int ret = 0;
	char buffer[BUFSIZ] = {0};
	struct csv_lifetime_t *pLIFE = &gPdct.lifetime;

	int len = 0;
	memset(buffer, 0, sizeof(buffer));

	len = snprintf(buffer, sizeof(buffer), 
		"total: %u mins \ncurrent: %u mins \nlast: %u mins \ncount: %u \n",
		pLIFE->total, pLIFE->current, pLIFE->last, pLIFE->count);
	ret = csv_file_write_data(pLIFE->file, (uint8_t *)buffer, len);
	if (ret < 0) {
//		log_info(LOG_FMT"ERROR : write file %s", LOG_ARGS, pLIFE->file);
		return -1;
	}

	sync();

	return 0;
}

// update total & current
int csv_life_update (void)
{
	struct csv_lifetime_t *pLIFE = &gPdct.lifetime;

	pLIFE->total++;
	pLIFE->current++;

	return file_lifetime_write();
}

// read when init ok
int csv_life_start (void)
{
	struct csv_lifetime_t *pLIFE = &gPdct.lifetime;

	pLIFE->last = pLIFE->current;	// 更新上次工时
	pLIFE->current = 0;
	pLIFE->count++;

	return file_lifetime_write();
}


// init lifetime file
int csv_life_init (void)
{
	int ret = 0;
	char buffer[BUFSIZ] = {0};
	struct csv_lifetime_t *pLIFE = &gPdct.lifetime;

	pLIFE->file = FILE_PATH_LIFETIME;
	pLIFE->total = 0;
	pLIFE->current = 0;
	pLIFE->last = 0;
	pLIFE->count = 0;

	ret = csv_file_read_string(pLIFE->file, buffer, sizeof(buffer));
	if (ret == 0) {
		pLIFE->total = utility_read_value(buffer, "total:");
		pLIFE->current = utility_read_value(buffer, "current:");
		pLIFE->last = utility_read_value(buffer, "last:");
		pLIFE->count = utility_read_value(buffer, "count:");
	} else {
		pLIFE->total = 0;
		pLIFE->current = 0;
		pLIFE->last = 0;
		pLIFE->count = 0;
		return file_lifetime_write();
	}

	return 0;
}





#ifdef __cplusplus
}
#endif

