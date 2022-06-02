#ifndef __CSV_FILE_H__
#define __CSV_FILE_H__

#ifdef __cplusplus
extern "C" {
#endif

/**@name	本地文件路径宏
* @{
*/
#define CONFIG_APPLICATION			"d3cam"

#define FILE_PATH_BIRTHDAY			("/home/rfid/birthday")	///< 设备生产年月日
#define FILE_PATH_UUID				("/home/rfid/uuid")


#define FILE_PATH_BACKTRACE			"log.log"

/** @}*/

/**@name	本地部分文件内容长度
* @{
*/
#define SIZE_BIRTHDAY				(8)
#define SIZE_UUID					(8)
/** @}*/


#define SIZE_BIRTHDAY				(8)
#define SIZE_UUID					(8)
#define SIZE_SERIAL_NO				(16)

/**@name	文件数据流结构体
* @{
*/
struct csv_file_t {
	char				*file_birthday;			///< 设备生产年月文件路径
	uint8_t				birthday[SIZE_BIRTHDAY+1];///< "20220530"年月日
	uint32_t			len_birthday;			///< 正常8个字节

	char				*file_uuid;				///< sn 设备序列号文件路径
	uint8_t				uuid[SIZE_UUID+1];		///< "CS000001"设备序列号内容
	uint32_t			len_uuid;				///< 设备序列号长度// 正常8字节

};
/** @}*/


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

extern uint8_t csv_file_isExist (char *path);

extern int csv_file_init (void);

#ifdef __cplusplus
}
#endif

#endif	/* __CSV_FILE_H__ */


