#ifndef __LOG_H__
#define __LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

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

extern int hex_printf (const uint8_t *buff, uint32_t count);

extern void log_do (int priority, const char *fmt, ...);

#ifdef CONFIG_DEBUG
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

  #define log_hex(buff, len, fmt, ...) log_do(LOG_DEBUG, "%s(%d): " fmt, \
		__func__, __LINE__, ##__VA_ARGS__); hex_printf(buff, len)

#else
  #define log_alert(fmt, ...) log_do(LOG_ALERT, "%lld[_A_]: " fmt, \
		utility_get_microsecond(), ##__VA_ARGS__)

  #define log_err(fmt, ...) log_do(LOG_ERR, "%lld[_E_]: " fmt, \
		utility_get_microsecond(), ##__VA_ARGS__)

  #define log_warn(fmt, ...) log_do(LOG_WARNING, "%lld[_W_]: " fmt, \
		utility_get_microsecond(), ##__VA_ARGS__)

  #define log_notice(fmt, ...) log_do(LOG_NOTICE, "%lld[_N_]: " fmt, \
		utility_get_microsecond(), ##__VA_ARGS__)

  #define log_info(fmt, ...) log_do(LOG_INFO, "%lld[_I_]: " fmt, \
		utility_get_microsecond(), ##__VA_ARGS__)

  #define log_debug(fmt, ...)

  #define log_hex(buff, len, fmt, ...)

#endif


#ifdef __cplusplus
}
#endif

#endif /* __LOG_H__ */

