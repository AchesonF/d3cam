#ifndef __LOG_H__
#define __LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

extern const char Hex2Ascii[17];

/*
* @param[in]	priority	日志级别
*		LOG_EMERG		system is unusable\n
*		LOG_ALERT		action must be taken immediately\n
*		LOG_CRIT		critical conditions\n
*		LOG_ERR			error conditions\n
*		LOG_WARNING		warning conditions\n
*		LOG_NOTICE		normal, but significant, condition\n
*		LOG_INFO		informational message\n
*		LOG_DEBUG		debug-level message\n
*/

extern int hex_printf (const uint8_t *buff, int count);

extern uint64_t utility_get_microsecond (void);

extern void log_do (int priority, const char *fmt, ...);

#if (CONFIG_DEBUG==1)
  #define log_alert(fmt, ...) log_do(LOG_ALERT, "%lld[_A_]: %s(%d): " fmt, \
		utility_get_microsecond(), __func__, __LINE__, ##__VA_ARGS__)

  #define log_err(fmt, ...) log_do(LOG_ERR, "%lld[_E_]: %s(%d): " fmt, \
		utility_get_microsecond(), __func__, __LINE__, ##__VA_ARGS__)

  #define log_warn(fmt, ...) log_do(LOG_WARNING, "%lld[_W_]: %s(%d): " fmt, \
		utility_get_microsecond(), __func__, __LINE__, ##__VA_ARGS__)

  #define log_notice(fmt, ...) log_do(LOG_NOTICE, "%lld[_N_]: %s(%d): " fmt, \
		utility_get_microsecond(), __func__, __LINE__, ##__VA_ARGS__)

  #define log_info(fmt, ...) log_do(LOG_INFO, "%lld[_I_]: %s(%d): " fmt, \
		utility_get_microsecond(), __func__, __LINE__, ##__VA_ARGS__)

  #define log_debug(fmt, ...) log_do(LOG_DEBUG, "%lld[_D_]: %s(%d): " fmt, \
		utility_get_microsecond(), __func__, __LINE__, ##__VA_ARGS__)

  #define log_hex(type, buff, len, fmt, ...) do {\
		if ((gPdct.tdata == type)||(gPdct.tdata == STREAM_ALL)) {	\
  			log_do(LOG_DEBUG, "%lld: %s(%d): " fmt, utility_get_microsecond(), \
  				__func__, __LINE__, ##__VA_ARGS__); \
  			hex_printf(buff, len); } } while(0)

#else
  #define log_alert(fmt, ...) log_do(LOG_ALERT, "[_A_]: " fmt, ##__VA_ARGS__)

  #define log_err(fmt, ...) log_do(LOG_ERR, "[_E_]: " fmt, ##__VA_ARGS__)

  #define log_warn(fmt, ...) log_do(LOG_WARNING, "[_W_]: " fmt, ##__VA_ARGS__)

  #define log_notice(fmt, ...) log_do(LOG_NOTICE, "[_N_]: " fmt, ##__VA_ARGS__)

  #define log_info(fmt, ...) log_do(LOG_INFO, "[_I_]: " fmt,  ##__VA_ARGS__)

  #define log_debug(fmt, ...)

  #define log_hex(type, buff, len, fmt, ...)

#endif


#ifdef __cplusplus
}
#endif

#endif /* __LOG_H__ */

