#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x0-0xF 的字符查找表*/
//const char Hex2Ascii[17] = "0123456789ABCDEF";
/* 0-9 数字转字符 */
const char Dec2Ascii[11] = "0123456789";


/* bcd 2 dec */
inline int convert_dec(uint8_t val)
{
	return (val / 16) * 10 + (val % 16);
}

/* dec 2 bcd */
inline uint8_t convert_hex(int val)
{
	return (val / 10) * 16 + (val % 10);
}

inline uint16_t swap16(uint16_t x)
{
	return ((x & 0x00FF) << 8) | 
		((x & 0xFF00) >> 8);
}

inline uint32_t swap32(uint32_t x)
{
	return ((x & 0x000000FF) << 24) |
    		((x & 0x0000FF00) << 8) |
    		((x & 0x00FF0000) >> 8) |
    		((x & 0xFF000000) >> 24);
}

// BE连续两个u8_t 字节转化为u16_t
uint16_t u8v_to_u16 (uint8_t *data)
{
	uint16_t *pe = (uint16_t *)data;

    return be16toh(*pe);

	//return ((uint16_t)(data[0]<<8)+data[1]);
}

// BE连续四个u8_t 字节转化为u32_t
uint32_t u8v_to_u32 (uint8_t *data)
{
	uint32_t *pe = (uint32_t *)data;
	return be32toh(*pe);

	//return ((uint32_t)(data[0]<<24)+(uint32_t)(data[1]<<16)
	//	+(uint32_t)(data[2]<<8)+data[3]);
}

// uint16_t 转化为2字节BE
uint8_t u16_to_u8v (uint16_t data, uint8_t *buf)
{
	*buf = (data>>8)&0xFF;
	*(buf+1) = data&0xFF;

	return 2;
}

// uint32_t 转化为4字节BE
uint8_t u32_to_u8v (uint32_t data, uint8_t *buf)
{
	*buf = (data>>24)&0xFF;
	*(buf+1) = (data>>16)&0xFF;
	*(buf+2) = (data>>8)&0xFF;
	*(buf+3) = data&0xFF;

	return 4;
}

// "a" -> 10
static uint8_t hexchar_to_decimal (char c)
{
    uint8_t ret = 0;

    if((c>='0')&&(c<='9')) {
        ret = c-'0';
    } else if((c>='a')&&(c<='f')) {
        ret = c-'a'+10;
    } else if((c>='A')&&(c<='F')) {
        ret = c-'A'+10;
    } else {
        ret = 0;
    }

    return ret;
}

// 0x2C ...  -> "2C..."
void u8v_to_hexstr (uint8_t *data, uint16_t len, char *buf)
{
	int i = 0;

	for(i = 0; i < len; i++) {
		buf[2*i] = Hex2Ascii[(data[i]>>4)&0x0F];
		buf[2*i+1] = Hex2Ascii[data[i]&0x0F];
	}
}

// "2C..." -> 0x2C ...
void hexstr_to_u8v (char *buf, uint16_t len, uint8_t *data)
{
	int i = 0;

	for(i = 0; i < len/2; i++) {
		data[i] = (hexchar_to_decimal(buf[2*i])<<4)+hexchar_to_decimal(buf[2*i+1]);
	}
}

double utility_get_sec_since_boot (void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);

	return (ts.tv_sec + (ts.tv_nsec / 1000.0 / 1000.0 / 1000.0));
}

/**
* @brief		获取1970.01.01 00:00:00.000 至今经过的毫秒数
* @param[in]	void
* @return		非0 : 毫秒
*/
uint64_t utility_get_millisecond (void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	return ((uint64_t)tv.tv_sec*1000llu+tv.tv_usec/1000); // ms
}

/**
 * @brief		Get 1970.01.01 00:00:00.000000  to now micro seconde number.
 * @param[in]	void
 * @return		!0 : micro second.
 */
/*uint64_t utility_get_microsecond (void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return ((uint64_t) tv.tv_sec * 1000000 + tv.tv_usec);
}
*/
int system_redef (const char *cmd)
{
	FILE *fp;
	int res = 0;
	char buf[1024] = {0};

	if (cmd == NULL) {
		return -1;
	}

	if ((fp = popen(cmd, "r")) == NULL) {
//		printf("ERROR : popen %s\n", strerror(errno));
		return -1;
	} else {
		memset(buf, 0, sizeof(buf));
		while (fgets(buf, sizeof(buf), fp)) {
			syslog(LOG_INFO, "%s\n", buf);
		}

		res = pclose(fp);
		if ( res == -1) {
//			printf("ERROR : pclose %s\n", strerror(errno));
			return res;
		}
	}

	return 0;
}

void utility_close (void)
{
	int i = 0, ret = 0;

	for (i = 255; i >= 0; i--) {
		ret = close(i);
		if (0 == ret) {
			printf("close fd(%d).\n", i);
		}
	}
}

static uint8_t utility_conv_month (char *month)
{
	uint8_t m = 1;
	if (strcasecmp("Jan", month) == 0) {
		m = 1;
	} else if (strcasecmp("Feb", month) == 0) {
		m = 2;
	} else if (strcasecmp("Mar", month) == 0) {
		m = 3;
	} else if (strcasecmp("Apr", month) == 0) {
		m = 4;
	} else if (strcasecmp("May", month) == 0) {
		m = 5;
	} else if (strcasecmp("Jun", month) == 0) {
		m = 6;
	} else if (strcasecmp("Jul", month) == 0) {
		m = 7;
	} else if (strcasecmp("Aug", month) == 0) {
		m = 8;
	} else if (strcasecmp("Sep", month) == 0) {
		m = 9;
	} else if (strcasecmp("Oct", month) == 0) {
		m = 10;
	} else if (strcasecmp("Nov", month) == 0) {
		m = 11;
	} else if (strcasecmp("Dec", month) == 0) {
		m = 12;
	}

	return m;
}

/* 获取应用编译时间 */
int utility_conv_buildtime (void)
{
	char temp[32] = {0};
	char month[6] = {0};
	int day = 0, year = 0, hour = 0, minute = 0, second = 0;
	uint8_t m = 1;
	struct tm tm_stamp;
	struct csv_product_t *pPdct = &gPdct;

	strcpy(temp, pPdct->app_buildtime);	// May 1 2022, 13:43:01
	sscanf(temp, "%s %d %d, %d:%d:%d", month, &day, &year, &hour, &minute, &second);

	m = utility_conv_month(month);

	char *buildtime = malloc(32);
	if (buildtime == NULL) {
//		log_err("ERROR : malloc");
		return -1;
	}

	memset(buildtime, 0, 32);

	tm_stamp.tm_year = year - 1900;
	tm_stamp.tm_mon = m - 1;
	tm_stamp.tm_mday = day;
	tm_stamp.tm_hour = hour;
	tm_stamp.tm_min = minute;
	tm_stamp.tm_sec = second;

	pPdct->build_timestamp = mktime(&tm_stamp);

	strftime(buildtime, 32, "%F %X", &tm_stamp);
	pPdct->app_buildtime = buildtime;

	snprintf(pPdct->app_info, 128, "%s %s (%s)", pPdct->app_name, pPdct->app_version, 
		pPdct->app_buildtime);

	return 0;
}

/* 获取内核编译信息 */
int utility_conv_kbuildtime (void)
{
	char month[6] = {0};
	char dummy[32] = {0};
	char tzone[16] = {0};
	char version[48] = {0};
	int day = 0, year = 0, hour = 0, minute = 0, second = 0;
	uint8_t m = 1;
	struct tm tm_stamp;
	struct utsname uts;
	struct csv_product_t *pPdct = &gPdct;

	// Linux HostPC 4.4.0-53-generic #74-Ubuntu SMP Fri Dec 2 15:59:10 UTC 2016 x86_64 x86_64 x86_64 GNU/Linux
	// Linux ITHINK 4.9.253-tegra #1 SMP PREEMPT Sun Apr 17 02:37:44 PDT 2022 aarch64 aarch64 aarch64 GNU/Linux
	// Linux debian 5.10.0-9-amd64 #1 SMP Debian 5.10.70-1 (2021-09-30) x86_64 GNU/Linux
	uname(&uts);

	memset(pPdct->kernel_version, 0, 132);
	memset(pPdct->kernel_buildtime, 0, 32);

	if (strstr(uts.version, "Debian") != NULL) {
		return -1;
	}
	
	// #74-Ubuntu SMP Fri Dec 2 15:59:10 UTC 2016 x86_64 x86_64 x86_64 GNU/Linux
	// #1 SMP PREEMPT Sun Apr 17 02:37:44 PDT 2022 aarch64 aarch64 aarch64 GNU/Linux
	if (strstr(uts.version, "PREEMPT") != NULL) {
		sscanf(uts.version, "#%s %s %s %s %s %d %d:%d:%d %s %d", version, dummy, dummy, dummy, 
			month, &day, &hour, &minute, &second, tzone, &year);
	} else {
		sscanf(uts.version, "#%s %s %s %s %d %d:%d:%d %s %d", version, dummy, dummy, 
			month, &day, &hour, &minute, &second, tzone, &year);
	}
	m = utility_conv_month(month);

	tm_stamp.tm_year = year - 1900;
	tm_stamp.tm_mon = m - 1;
	tm_stamp.tm_mday = day;
	tm_stamp.tm_hour = hour;
	tm_stamp.tm_min = minute;
	tm_stamp.tm_sec = second;

	snprintf(pPdct->kernel_version, 132, "%s %s #%s", uts.sysname, uts.release, version);
	strftime(pPdct->kernel_buildtime, 32, "%F %X", &tm_stamp);
	strcat(pPdct->kernel_buildtime, tzone);

	return 0;
}

/* 启动时调整系统时钟 不早于编译时间 */
int utility_calibrate_clock (void)
{
	struct tm *tm;
	char timebuf[64] = {0};
	struct csv_product_t *pPdct = &gPdct;

	memset(timebuf, 0, 64);
	time_t now = time(NULL);
	tm = localtime(&now);
	if (strftime(timebuf, sizeof(timebuf), "%F %X", tm) != 0) {
		log_info("Boot time : %s ", timebuf);
	}

	struct timeval tv;
	gettimeofday(&tv, NULL);

	if (tv.tv_sec < pPdct->build_timestamp) {
		log_info("calibrate system clock to default '%s'", pPdct->app_buildtime);

		char date_buf[40] = {0};
		memset(date_buf, 0, 40);
		snprintf(date_buf, 40, "date -s \"@%d\"", pPdct->build_timestamp);
		system_redef(date_buf);
		system_redef("hwclock -w -u");
	}

	return 0;
}


uint32_t utility_read_value (const char *buffer, const char *prefix)
{
	buffer = strstr(buffer, prefix);

	if (buffer != NULL) {
		buffer += strlen(prefix);
		while (isspace(*buffer)) {
			buffer++;
		}
		return (*buffer != '\0') ? strtoul(buffer, NULL, 0) : 0;
	} else {
		return 0;
	}
}

void utility_read_values (const char *buffer, const char *prefix, uint32_t *values, int count)
{
	int i = 0;

	buffer = strstr(buffer, prefix);

	if (buffer != NULL) {
		buffer += strlen(prefix);
		for (i = 0; i < count; i++) {
			while (isspace(*buffer)) {
				buffer++;
			}

			if (*buffer != '\0') {
				values[i] = strtoul(buffer, (char **)&buffer, 0);
			} else {
				values[i] = 0;
			}
		}
	} else {
		memset(values, 0, count * sizeof(uint32_t));
	}
}

/* *path : 路径
group : 次数
idx : 编号
lr : 左右
suffix : 后缀类型
*img_file : 生成名
*/
int generate_image_filename (char *path, uint16_t group, 
	int idx, int lr, char *img_file)
{
	if (idx == 0) {
		snprintf(img_file, 128, "%s/CSV_%03dC%d.bmp", path, group, lr+1);
	} else {
		snprintf(img_file, 128, "%s/CSV_%03dC%dS00P%03d.bmp", path, group, lr+1, idx);
	}

	log_debug("img : '%s'", img_file);

	return 0;
}





#ifdef __cplusplus
}
#endif


