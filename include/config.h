#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

// define in cmake
#define CONFIG_APP_NAME			PROJECT_UPPER_NAME
#define SOFTWARE_VERSION		PROJECT_VERSION

#define BUILD_DATE				__DATE__
#define BUILD_TIME				__TIME__

#define SOFT_BUILDTIME			(BUILD_DATE ", " BUILD_TIME)	///< 程序编译时间
#define COMPILER_VERSION		__VERSION__


#ifndef CONFIG_DEBUG
  #define CONFIG_DEBUG			0
#endif





#ifdef __cplusplus
}
#endif

#endif /* __CONFIG_H__ */


