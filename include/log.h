#ifndef __LOG_H__
#define __LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

#if (CONFIG_DEBUG_HEX_FORMAT==1)
  #define HEX_SHOW_FORMAT	1
#endif

#define MAX_LINE					(4096)		///< 每行最大字符数

extern const char Hex2Ascii[17];

/*
* @param[in]	priority	日志级别
*		LOG_EMERG		system is unusable (系统无法使用)
*		LOG_ALERT		action must be taken immediately (操作必须立即执行)
*		LOG_CRIT		critical conditions (紧急条件)
*		LOG_ERR			error conditions (错误条件)
*		LOG_WARNING		warning conditions (警告条件)
*		LOG_NOTICE		normal, but significant, condition (正常但重要的条件)
*		LOG_INFO		informational message (信息)
*		LOG_DEBUG		debug-level message (调试级别的消息)
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

