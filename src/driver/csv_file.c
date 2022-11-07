#include "inc_files.h"


#ifdef __cplusplus
extern "C" {
#endif

struct csv_file_t gFILE;

/**
* @brief		获取指定文件的大小
* @param[in]	*path		文件名
* @param[out]	*filesize	文件大小
* @return		0	: 获取成功\n
				<0	: 获取失败
*/
int csv_file_get_size (const char *path, uint32_t *filesize)
{
	FILE *fp = NULL;

	if (NULL == path) {
		log_warn("ERROR : %s null path.",  __func__);

		return -1;
	}

	fp = fopen(path, "r");
	if (fp == NULL) {
		log_err("ERROR : open '%s'.", path);

		return -2;
	}

	fseek(fp, 0L, SEEK_END);
	*filesize = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	fclose(fp);

	return 0;
}

/**
* @brief		读取指定文件指定长度的内容
* @param[in]	*path		文件名
* @param[in]	size		长度
* @param[out]	*buf		内容缓冲区
* @return		0	: 读取成功\n
				<0	: 读取失败
*/
// 读取固定长度文件流
int csv_file_read_data (const char *path, uint8_t *buf, uint32_t size)
{
	int ret = 0;
	FILE *fp = NULL;

	if (size == 0) {
		log_warn("ERROR : %s size 0.", __func__);

		return -1;
	}

	fp = fopen(path, "r");
	if (fp == NULL) {
		log_err("ERROR : fopen '%s'.", path);

		return -2;
	}

	fseek(fp, 0L, SEEK_SET);

	ret = fread(buf, size, 1, fp);
	if (ret < 0) {
		log_err("ERROR : fread ret[%ld] vs size[%ld].", ret, size);

		fclose(fp);
		return -3;
	}

	fclose(fp);

	return 0;
}

// 读取不定长度文件流
int csv_file_read_string (const char *filename, char *buf, size_t size)
{
	FILE *fp = NULL;
	size_t rv = 0;

	fp = fopen(filename, "r");
	if (fp == NULL) {
		log_err("ERROR : fopen '%s'.", filename);
		return -1;
	}

	rv = fread(buf, 1, size - 1, fp);
	if (rv <= 0) {
		log_err("ERROR : fread '%s'.", filename);
		
		fclose(fp);
		return -1;
	}

	buf[rv] = '\0';

	if (fclose(fp) != 0) {
		log_err("ERROR : fclose '%s'.", filename);
		return -1;
	}

	return 0;
}

/**
* @brief		写入指定文件指定长度的内容
* @param[in]	*path		文件名
* @param[in]	size		长度
* @param[out]	*buf		内容缓冲区
* @return		0	: 写入成功\n
				<0	: 写入失败
*/
int csv_file_write_data (const char *path, uint8_t *buf, uint32_t size)
{
	int ret = 0;
	FILE *fp = NULL;

	if (size==0) {
		log_warn("ERROR : %s size 0.", __func__);
		
		return -1;
	}

	fp = fopen(path, "w");
	if (fp == NULL) {
		log_err("ERROR : open '%s'.", path);

		return -2;
	}

	ret = fwrite(buf, size, 1, fp);
	if (ret < 0) {
		log_err("ERROR : fwrite.");
		fclose(fp);
		return -3;
	}

	fclose(fp);

	return 0;
}

int csv_file_write_data_append (const char *path, uint8_t *buf, uint32_t size)
{
	int ret = 0;
	FILE *fp = NULL;

	if (size==0) {
		log_warn("ERROR : %s size 0.", __func__);
		
		return -1;
	}

	fp = fopen(path, "a");
	if (fp == NULL) {
		log_err("ERROR : open '%s'.", path);

		return -2;
	}

	ret = fwrite(buf, size, 1, fp);
	if (ret < 0) {
		log_err("ERROR : fwrite.");
		fclose(fp);
		return -3;
	}

	fclose(fp);

	return 0;
}

/*
	st.st_mode:
S_IFMT   00170000 mask

S_IFSOCK 0140000 "socket"
S_IFLNK	 0120000 "symlink"
S_IFREG  0100000 "regular file"
S_IFBLK  0060000 "block device"
S_IFDIR  0040000 "directory"
S_IFCHR  0020000 "character device"
S_IFIFO  0010000 "FIFO/pipe"
S_ISUID  0004000
S_ISGID  0002000
S_ISVTX  0001000

*/
uint8_t csv_file_isPath (char *path, uint32_t mode)
{
	struct stat st;
	int ret = 0;

	if (NULL == path) {
		return false;
	}

	ret = stat(path, &st);
	if ((0 == ret)&&(mode == (st.st_mode & S_IFMT))) {
		return true;
	}

	return false;
}

int csv_file_mkdir (char *dir)
{
	int ret = -1;
	char str_cmd[512] = {0};

	memset(str_cmd, 0, 512);

	if ((NULL != dir)&&(!csv_file_isPath(dir, S_IFDIR))) {
		snprintf(str_cmd, 512, "mkdir -p %s", dir);
		ret = system(str_cmd);
	}

	return ret;
}

uint32_t csv_file_modify_time (char *path)
{
	int ret = 0;
	struct stat st;

	if (NULL == path) {
		return 0;
	}

	ret = stat(path, &st);
	if (0 == ret) {
		return st.st_mtim.tv_sec;
	}

	return 0;
}

static int csv_file_get (struct csv_file_t *pFILE)
{
	int ret = 0;
	uint32_t len_file = 0;
	char str_cmd[512] = {0};

	// file 1
	if (!csv_file_isPath(pFILE->udpserv, S_IFREG)) {
		memset(str_cmd, 0, 512);
		snprintf(str_cmd, 512, "echo \"%s:%d\" > %s", 
			DEFAULT_LOG_SERV, DEFAULT_LOG_PORT, pFILE->udpserv);
		system(str_cmd);
	}

	ret = csv_file_get_size(pFILE->udpserv, &len_file);
	if ((0 == ret)&&(len_file > 0)&&(len_file <= 22)) {
		uint8_t *buf_file = (uint8_t *)malloc(len_file);
		if (NULL != buf_file) {
			int nget = 0;
			char str_ip[32] = {0};
			char str_port[32] = {0};
			int port = 0;
			ret = csv_file_read_data(pFILE->udpserv, buf_file, len_file);

			nget = sscanf((char *)buf_file, "%[^:]:%[^:]", str_ip, str_port);
			if (2 == nget) {
				ret = check_user_ip(str_ip);
				if (0 == ret) {
					strcpy(gUDP.ip, str_ip);
				}
				port = atoi(str_port);
				if (port > 1024) {
					gUDP.port = port;
				}
				gUDP.reinit = 1;
				//printf("log server : '%s:%d'.\n", gUDP.ip, gUDP.port);
			}

			free(buf_file);
		}
	}

	// file 2
	if (!csv_file_isPath(pFILE->heartbeat_cfg, S_IFREG)) {
		memset(str_cmd, 0, 512);
		snprintf(str_cmd, 512, "echo \"0:3000\" > %s", pFILE->heartbeat_cfg);
		system(str_cmd);
	}

	ret = csv_file_get_size(pFILE->heartbeat_cfg, &len_file);
	if ((0 == ret)&&(len_file > 0)&&(len_file <= 12)) {
		uint8_t *buf_file = (uint8_t *)malloc(len_file);
		if (NULL != buf_file) {
			int nget = 0;
			char str_enable[12] = {0};
			char str_period[12] = {0};
			
			ret = csv_file_read_data(pFILE->heartbeat_cfg, buf_file, len_file);

			nget = sscanf((char *)buf_file, "%[^:]:%[^:]", str_enable, str_period);
			if (2 == nget) {
				if (atoi(str_enable)) {
					pFILE->beat_enable = 1;
				} else {
					pFILE->beat_enable = 0;
				}

				if (atoi(str_period) > 100) { // at least 0.1s
					pFILE->beat_period = atoi(str_period);
				} else {
					pFILE->beat_period = 3000;
				}

				//printf("heartbeat cfg : '%d:%d'.\n", pFILE->beat_enable, pFILE->beat_period);
			}

			free(buf_file);
		}
	}

	return ret;
}

/**
* @brief		将数据流写入文件
* @param[in]	*buf	待写入数据
* @param[in]	*fp		文件描述符
* @param[in]	size	待写入数据长度
* @return		0	: 写入成功\n
				<0	: 写入失败
*/
int file_write_data(char *buf, FILE *fp, uint32_t size)
{
	int ret = 0;
	int nwr = 0;

	if (fp == NULL || size == 0) {
		return -1;
	}

	nwr = fwrite(buf, size, 1, fp);
	if (nwr < 0) {
		ret = -1;
	}

	return ret;
}

int csv_file_init (void)
{
	int i = 0;
	char str_dir[256]={0};
	char str_cmd[512] = {0};
	char dir_sets[5][128] = {
		PATH_CONFIG_FILES,
		PATH_SETS_FILES,
		PATH_WEB_FILES,
		PATH_DATA_FILES,
		PATH_MODEL_FILES
	};

	struct csv_file_t *pFILE = &gFILE;

	memset(pFILE, 0, sizeof(struct csv_file_t));

	for (i = 0; i < 5; i++) {
		memset(str_dir, 0, 256);
		snprintf(str_dir, 256, "%s/", dir_sets[i]);
		if (!csv_file_isPath(str_dir, S_IFDIR)) {
			memset(str_cmd, 0, 512);
			snprintf(str_cmd, 512, "mkdir -p %s", str_dir);
			system(str_cmd);
		}
	}

	memset(str_dir, 0, 256);
	snprintf(str_dir, 256, "%s/", PATH_LOG_FILES);
	if (!csv_file_isPath(str_dir, S_IFLNK)) {
		memset(str_cmd, 0, 512);
		snprintf(str_cmd, 512, "ln -s /var/log %s", str_dir);
		system(str_cmd);
	}

	snprintf(pFILE->udpserv, 256, "%s", FILE_UDP_SERVER);
	snprintf(pFILE->heartbeat_cfg, 256, "%s", FILE_CFG_HEARTBEAT);

	return csv_file_get(pFILE);
}

#ifdef __cplusplus
}
#endif


