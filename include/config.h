#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif


#define CONFIG_APP_NAME			PROJECT_UPPER_NAME
#define SOFTWARE_VERSION		PROJECT_VERSION

#define SOFT_BUILDTIME			(__DATE__", "__TIME__)	///< 程序编译时间
#define COMPILER_VERSION		__VERSION__


#if (BUILD_TYPE_DEBUG==1)
  #define CONFIG_DEBUG 0
#endif





#ifdef __cplusplus
}
#endif

#endif /* __CONFIG_H__ */


