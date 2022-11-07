#ifndef __CSV_FILE_H__
#define __CSV_FILE_H__

#ifdef __cplusplus
extern "C" {
#endif

/**@name	本地文件路径宏
* @{
*/
#define CONFIG_APPLICATION			"d3cam"

#define PATH_CONFIG_FILES			"config"	///< 配置文件 csvcfg.xml udpserv calib.xml
#define PATH_SETS_FILES				"sets"		///< 固化文件 gev.xml(zip) .cer .key
#define PATH_WEB_FILES				"web"		///< web html
#define PATH_DATA_FILES				"data"		///< images(calib, depth, hdr)
#define PATH_MODEL_FILES			"model"		///< for so use
#define PATH_LOG_FILES				"log"		///< link to syslog

#define FILE_UDP_SERVER				(PATH_CONFIG_FILES "/udpserv")	// "127.0.0.1:36666"
#define FILE_CFG_HEARTBEAT			(PATH_CONFIG_FILES "/hbcfg")	// "1:3000"

#define FILE_PATH_BACKTRACE			("backtrace.log")

#define FILE_CALIB_XML				(PATH_CONFIG_FILES "/CSV_Cali_DaHengCamera.xml")
#define FILE_CALIB_XML_NEW			(PATH_CONFIG_FILES "/calib_new.xml")


/** @}*/


/**@name	文件数据流结构体
* @{
*/
struct csv_file_t {
	char				udpserv[256];
	char				heartbeat_cfg[256];

	uint8_t				beat_enable;
	uint32_t			beat_period;

};
/** @}*/

extern struct csv_file_t gFILE;

/* 获取指定文件的大小 */
extern int csv_file_get_size (const char *path, uint32_t *filesize);

/* 读取指定文件指定长度的内容 */
extern int csv_file_read_data (const char *path, uint8_t *buf, uint32_t size);

/* 读取制定文件不定长度内容 */
extern int csv_file_read_string (const char *filename, char *buf, size_t size);

/* 写入指定文件指定长度的内容 */
extern int csv_file_write_data (const char *path, uint8_t *buf, uint32_t size);

extern int csv_file_write_data_append (const char *path, uint8_t *buf, uint32_t size);

/* 将数据流写入文件 */
extern int file_write_data (char * buf, FILE * fp, uint32_t size);

extern uint8_t csv_file_isPath (char *path, uint32_t mode);

extern int csv_file_mkdir (char *dir);

extern uint32_t csv_file_modify_time (char *path);

extern int csv_file_init (void);

#ifdef __cplusplus
}
#endif

#endif	/* __CSV_FILE_H__ */


