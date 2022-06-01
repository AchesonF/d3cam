#include "inc_files.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
* @brief		获取指定文件的大小
* @param[in]	*path		文件名
* @param[out]	*filesize	文件大小
* @return		0	: 获取成功\n
				<0	: 获取失败
*/
int csv_file_get_size (const char *path, uint32_t *filesize)
{
	FILE *fp=NULL;

	if (path==NULL) {
//		log_info(LOG_FMT"ERROR : %s", LOG_ARGS, __func__);

		return -1;
	}

	fp = fopen(path, "r");
	if (fp == NULL) {
#ifdef CONFIG_DEBUG
//		log_err(LOG_FMT"ERROR : open \"%s\"", LOG_ARGS, path);
#endif
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
//		log_info(LOG_FMT"ERROR : %s", LOG_ARGS, __func__);

		return -1;
	}

	fp = fopen(path, "r");
	if (fp == NULL) {
#ifdef CONFIG_DEBUG
//		log_err(LOG_FMT"ERROR : fopen \"%s\"", LOG_ARGS, path);
#endif
		return -2;
	}

	fseek(fp, 0L, SEEK_SET);

	ret = fread(buf, size, 1, fp);
	if (ret < 0) {
//		log_err(LOG_FMT"ERROR : fread ret[%ld] vs size[%ld]", LOG_ARGS, ret, size);

		fclose(fp);
		return -3;
	}

	fclose(fp);

	return 0;
}

// 读取不定长度文件流
int csv_file_read_string (const char *filename, char *buf, size_t size)
{
	FILE *fp;
	size_t rv;

	fp = fopen(filename, "r");
	if (fp == NULL) {
//		log_err(LOG_FMT"ERROR : fopen \"%s\"", LOG_ARGS, filename);
		return -1;
	}

	rv = fread(buf, 1, size - 1, fp);
	if (rv <= 0) {
//		log_err(LOG_FMT"ERROR : fread \"%s\"", LOG_ARGS, filename);
		
		fclose(fp);
		return -1;
	}

	buf[rv] = '\0';

	if (fclose(fp) != 0) {
//		log_err(LOG_FMT"ERROR : fclose \"%s\"", LOG_ARGS, filename);
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
	int ret=0;
	FILE *fp=NULL;

	if (size==0) {
//		log_info(LOG_FMT"ERROR : %s", LOG_ARGS, __func__);
		
		return -1;
	}

	fp = fopen(path, "w");
	if (fp == NULL) {
#ifdef CONFIG_DEBUG
//		log_err(LOG_FMT"ERROR : open \"%s\"", LOG_ARGS, path);
#endif
		return -2;
	}

	ret = fwrite(buf, size, 1, fp);
	if (ret < 0) {
//		log_err(LOG_FMT"ERROR : fwrite", LOG_ARGS);
		fclose(fp);
		return -3;
	}

	fclose(fp);

	return 0;
}

int csv_file_write_data_append (const char *path, uint8_t *buf, uint32_t size)
{
	int ret=0;
	FILE *fp=NULL;

	if (size==0) {
//		log_info(LOG_FMT"ERROR : %s", LOG_ARGS, __func__);
		
		return -1;
	}

	fp = fopen(path, "a");
	if (fp == NULL) {
#ifdef CONFIG_DEBUG
//		log_err(LOG_FMT"ERROR : open \"%s\"", LOG_ARGS, path);
#endif
		return -2;
	}

	ret = fwrite(buf, size, 1, fp);
	if (ret < 0) {
//		log_err(LOG_FMT"ERROR : fwrite", LOG_ARGS);
		fclose(fp);
		return -3;
	}

	fclose(fp);

	return 0;
}

uint8_t csv_file_isExist (char *path)
{
	struct stat st;
	int ret = 0;

	ret = stat(path, &st);
	if (0 == ret) {
		return true;
	}

	return false;
}

static int csv_file_get (struct csv_file_t *pFILE)
{
	int ret = 0;
/*	char str_cmd[256] = {0};

	// birthday
	if (uhf_file_isExist(pFILE->file_birthday)) {
		ret = uhf_file_get_size(pFILE->file_birthday, &pFILE->len_birthday);
		if ((ret < 0)||(pFILE->len_birthday < SIZE_BIRTHDAY)) {
			pFILE->len_birthday = SIZE_BIRTHDAY;
			memcpy(pFILE->birthday, DEFAULT_BIRTHDAY, pFILE->len_birthday);
		} else {
			pFILE->len_birthday = SIZE_BIRTHDAY;	// 确保只读取前8个字节
			uhf_file_read_data(pFILE->file_birthday, pFILE->birthday, pFILE->len_birthday);
		}
	} else {
		memset(str_cmd, 0, 256);
		snprintf(str_cmd, 256, "echo %s > %s", DEFAULT_BIRTHDAY, FILE_PATH_BIRTHDAY);
		system_redef(str_cmd);
		pFILE->len_birthday = SIZE_BIRTHDAY;
		memcpy(pFILE->birthday, DEFAULT_BIRTHDAY, pFILE->len_birthday);
		sync();
	}

	// sn = uuid
	if (uhf_file_isExist(pFILE->file_uuid)) {
		ret = uhf_file_get_size(pFILE->file_uuid, &pFILE->len_uuid);
		if ((ret < 0)||(pFILE->len_uuid < SIZE_UUID)) {
			pFILE->len_uuid = SIZE_UUID;
			memcpy(pFILE->uuid, DEFAULT_UUID, pFILE->len_uuid);
			pFILE->uuid[SIZE_UUID] = 0x00;
		} else {
			pFILE->len_uuid = SIZE_UUID;	// 确保只读取前8个字节
			uhf_file_read_data(pFILE->file_uuid, pFILE->uuid, pFILE->len_uuid);
			pFILE->uuid[SIZE_UUID] = 0x00;
		}
	} else {
		memset(str_cmd, 0, 256);
		snprintf(str_cmd, 256, "echo %s > %s", DEFAULT_UUID, FILE_PATH_UUID);
		system_redef(str_cmd);
		pFILE->len_uuid = SIZE_UUID;
		memcpy(pFILE->uuid, DEFAULT_UUID, pFILE->len_uuid);
		pFILE->uuid[SIZE_UUID] = 0x00;
	}
*/
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
	struct csv_file_t *pFILE = &gCSV->file;

	pFILE->file_birthday = FILE_PATH_BIRTHDAY;
	pFILE->file_uuid = FILE_PATH_UUID;

	return csv_file_get(pFILE);
}

#ifdef __cplusplus
}
#endif


