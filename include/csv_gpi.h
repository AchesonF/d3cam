#ifndef __CSV_GPI_H__
#define __CSV_GPI_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NAME_INPUT					("'csv_input'")


#define DEV_GPIO_INPUT				("/dev/input/by-path/platform-gpio-keys-event")


// gpio-key KEY_SLEEP(142),KEY_POWER(116) define in file "tegra194-p3509-0000+p3668-0000.dts"
#define GPI_IN_FORCE				(0x101)		///< Force Recovery (G, 0)
#define GPI_IN_POWER				(0x74)		///< Power	(EE, 4)

#define GPI_LOW						(0)			///< GPI输入为0
#define GPI_HIGH					(1)			///< GPI输入为1

enum {
	IDX_IN_1				= (0x00),
	IDX_IN_2				= (0x01),

	TOTAL_INPUTS
};


struct csv_gpi_t {
	char				*name;					///< 名称
	char				*dev;					///< 输入接口地址
	int					fd;						///< 输入设备描述符

	struct input_event	key;
	int					code;					///< 键值编号
	int					value;					///< 状态值

	uint8_t				status[TOTAL_INPUTS];	///< 输入模拟量高/低
};

extern int csv_gpi_trigger (struct csv_gpi_t *pGPI);

extern int csv_gpi_init (void);

extern int csv_gpi_deinit (void);


#ifdef __cplusplus
}
#endif

#endif /* __CSV_GPI_H__ */

