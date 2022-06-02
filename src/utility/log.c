#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_LINE					(4096)		///< 每行最大字符数

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

