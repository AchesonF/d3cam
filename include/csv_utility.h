#ifndef __CSV_UTILITY_H__
#define __CSV_UTILITY_H__

#ifdef __cplusplus
extern "C" {
#endif

#define toSTR(_str)			#_str

//extern const char Hex2Ascii[17];
extern const char Dec2Ascii[11];

extern int convert_dec(uint8_t val);
extern uint8_t convert_hex(int val);


extern uint16_t swap16 (uint16_t x);
extern uint32_t swap32 (uint32_t x);


//#define MIN(a,b) (((a)>(b))?(b):(a))
//#define MAX(a,b) (((a)>(b))?(a):(b))

extern uint16_t u8v_to_u16(uint8_t *data);

extern uint32_t u8v_to_u32(uint8_t *data);

extern uint8_t u16_to_u8v(uint16_t data, uint8_t *buf);

extern uint8_t u32_to_u8v(uint32_t data, uint8_t *buf);

extern void u8v_to_hexstr (uint8_t *data, uint16_t len, char *buf);

extern void hexstr_to_u8v (char *buf, uint16_t len, uint8_t *data);

extern double utility_get_sec_since_boot (void);

extern uint64_t utility_get_millisecond (void);

extern int system_redef (const char *cmd);

extern void utility_close (void);

/* 转换编译时间显示格式 */
extern int utility_conv_buildtime (void);

extern int utility_conv_kbuildtime (void);

extern int utility_calibrate_clock (void);

extern uint32_t utility_read_value (const char *buffer, const char *prefix);
extern void utility_read_values (const char *buffer, const char *prefix, uint32_t *values, int count);

extern int generate_image_filename (char *path, uint16_t group, 
	int idx, int lr, uint8_t suffix, char *img_file);

#ifdef __cplusplus
}
#endif

#endif /* __CSV_UTILITY_H__ */




