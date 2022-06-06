#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_LINE					(4096)		///< 每行最大字符数

int hex_printf (const uint8_t *buff, int count)
{
	uint32_t i = 0, j = 0;
	char str[16] = {0};
	char str_val[75] = {0};
	uint32_t index = 0;
	uint32_t str_index = 0;
	uint32_t cnt = (count+15)>>4;

	if (count < 0) {
		return -1;
	}

	printf("offs  01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0F 0E 0F |     ASCII\n");
	printf("------------------------------------------------------------------------\n");
	for (i = 0; i < cnt; i++) {
		index = 0;
		str_index = 0;
		str_val[index++] = Hex2Ascii[(i & 0xF000) >> 12];
		str_val[index++] = Hex2Ascii[(i & 0xF00) >> 8];
		str_val[index++] = Hex2Ascii[(i & 0xF0) >> 4];
		str_val[index++] = Hex2Ascii[i & 0xF];
		str_val[index++] = ' ';
		str_val[index++] = ' ';

		for (j = 0; j < 16; j++) {
			if (j + (i << 4) < count) {
				str_val[index++] = Hex2Ascii[(buff[j + (i << 4)] & 0xF0) >> 4];
				str_val[index++] = Hex2Ascii[buff[j + (i << 4)] & 0xF];
				str_val[index++] = ' ';
				if ((buff[j + (i << 4)] >= 0x20) && (buff[j + (i << 4)] <= 0x7E)) {
					str[str_index++] = buff[j + (i << 4)];
				} else {
					str[str_index++] = '.';
				}
			} else {
				str_val[index++] = ' ';
				str_val[index++] = ' ';
				str_val[index++] = ' ';
				str[str_index++] = ' ';
			}
		}
		str_val[index++] = ' ';
		str_val[index++] = ' ';
		memcpy(&str_val[index], str, str_index);
		index += str_index;
		str_val[index++] = '\n';
		str_val[index++] = '\0';
		printf("%s", str_val);
	}
	printf("------------------------------------------------------------------------\n\n");

	return 0;
}



static void log_printf (int priority, const char *fmt, va_list ap)
{
	int errno_save = errno;
	char buf[MAX_LINE]={0};

	vsnprintf(buf, MAX_LINE, fmt, ap);

	if (LOG_ERR == priority) {				// add error tips
		snprintf(buf+strlen(buf), MAX_LINE-1-strlen(buf), 
			" : [%d]%s", errno_save, strerror(errno_save));
	}
	strcat(buf, "\n");
	syslog(priority, "%s", buf);

	if (gPdct.tlog) {
		fflush(stdout);
		fputs(buf, stderr);
		fflush(stderr);
	}
}

void log_do (int priority, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_printf(priority, fmt, ap);
	va_end(ap);
}



#ifdef __cplusplus
}
#endif

