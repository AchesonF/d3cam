#ifndef __CSV_TYPE_H__
#define __CSV_TYPE_H__



typedef enum {
	CONNECT_NULL					= (0x00000000),	// 空值
	CONNECT_DEVICE_ENUM				= (0x00000001),	// 连接设备
	DISCONNECT_DEVICE_ENUM			= (0x00000002),	// 断开设备

	CAMERA_ENMU						= (0x00001000),	// 相机枚举

	CAMERA_CONNECT					= (0x00002000),	// 相机连接
	CAMERA_DISCONNECT				= (0x00002001),	// 断开相机

	CAMERA_GET_INFO					= (0x00003000),

	CAMERA_GET_GAIN					= (0x00003002),	// 获取增益
	CAMERA_GET_CALIB_FILE			= (0x00003003),	// 获取标定文件
	CAMERA_GET_CAMERA_NAME			= (0x00003004),	// 获取相机名称
	CAMERA_GET_RATE					= (0x00003010),	// 获取帧率

	CAMERA_GET_EXPOSURE				= (0x00003001),	// 获取曝光时间(us),常亮图(0x05)(标定前/高速识图前适用)
	CAMERA_GET_BRIGHTNESS			= (0x00003011),	// 获取亮度(标定条纹图用)
	CAMERA_GET_BRIGHTNESS_PC		= (0x00003012),	// 获取亮度(高速识图用)
	CAMERA_GET_EXPOSURE_CALIB		= (0x00003013),	// 获取曝光时间(us),(标定条纹图适用)
	CAMERA_GET_EXPOSURE_PC			= (0x00003014),	// 获取曝光时间(us),(识图条纹图适用)

	CAMERA_SET_EXPOSURE				= (0x00004001),	// 设置曝光时间(us)(4字节浮点),常亮图(0x05)(标定前/高速识图前适用)
	CAMERA_SET_BRIGHTNESS			= (0x00004011),	// 设置亮度(标定条纹图用)(4字节浮点)
	CAMERA_SET_BRIGHTNESS_PC		= (0x00004012),	// 设置亮度(高速识图用)(4字节浮点)
	CAMERA_SET_EXPOSURE_CALIB		= (0x00004013),	// 设置曝光时间(us)(4字节浮点),(标定条纹图适用)
	CAMERA_SET_EXPOSURE_PC			= (0x00004014),	// 设置曝光时间(us)(4字节浮点),(识图条纹图适用)

	CAMERA_GET_CALIBRATE			= (0x00005009),	// 获取标定图 (1+22)*2
	CAMERA_GET_POINTCLOUD			= (0x0000500A),	// 获取高速识图 1*2+1[13*2]

	CAMERA_SET_INFO					= (0x00004000),	// 空值

	CAMERA_SET_GAIN					= (0x00004002),	// 设置相机增益
	CAMERA_SET_CALIB_FILE			= (0x00004003),	// 接收上位机传过来的标定文件
	CAMERA_SET_CAMERA_NAME			= (0x00004004),	// 设置相机名称
	CAMERA_SET_LED_DELAY_TIME		= (0x00004005),	// 设置光机点亮延迟
	CAMERA_SET_RGB_EXPOSURE			= (0x00004006),	// 设置RGB图像的参数
	CAMERA_SET_LEFT_RIGHT_TYPE		= (0x00004007),	// 设置相机左右对调
	CAMERA_SET_CONT_EXPOSURE		= (0x00004008),	// 设置相机曝光参数
	CAMERA_SET_IMG_TYPE				= (0x00004009),	// 设置相机图像格式
	CAMERA_SET_RATE					= (0x00004010),	// 设置帧率
	CAMERA_PARAM_SAVE				= (0x00004100),	// 保存相机参数到配置文件里

	CAMERA_GET_GRAB_FLASH			= (0x00005000),	// 获取带随机光的图像,闪动
	CAMERA_GET_GRAB_LEFT			= (0x00005001),
	CAMERA_GET_GRAB_RIGHT			= (0x00005002),
	CAMERA_GET_GRAB_DEEP			= (0x00005003),	// 获取深度图
	CAMERA_GET_GRAB_LEDOFF			= (0x00005004),	// 光机关闭
	CAMERA_GET_GRAB_LEDON			= (0x00005005),	// 光机常亮
	CAMERA_GET_GRAB_RGB				= (0x00005006),	// 取相机彩色图像,双图
	CAMERA_GET_GRAB_RGB_LEFT		= (0x00005007),	// 取相机彩色图像,左图
	CAMERA_GET_GRAB_RGB_RIGHT		= (0x00005008),	// 取相机彩色图像,右图


	CAMERA_GET_STREAM				= (0x00006000),
	CAMERA_GET_STREAM_LEFT			= (0x00006001),
	CAMERA_GET_STREAM_RIGHT			= (0x00006002),

	STEREO_PARAM					= (0x00007000),	// 设置双目匹配参数
	STEREO_PARAM_NUMDISPARITIES		= (0x00007001),
	STEREO_PARAM_UNIQRATIO			= (0x00007002),	// 防止误匹配参数
	STEREO_PARAM_BLOCKSIZE			= (0x00007003),	// 设置blocksize
	STEREO_PARAM_SAVE				= (0x00007100),	// 将立体匹配的参数保存成文件

	CAMERA_CHECK					= (0x00010000),
	SYS_REBOOT						= (0x00020000),	// 重新启动系统
	SYS_POWEROFF					= (0x00020001),	// 关闭系统，需要重新上电才能打开
	SYS_SET_ERRLOG_LEVEL			= (0x00020002),	// 打开网络调试模式
	SYS_SOFT_INFO					= (0x00020004),	// 系统运行的软件的编译或版本信息
	SYS_DEV_INFO					= (0x00020005),	// 硬件信息
	SYS_FIRMWARE_UPDATE				= (0x00020006),	// cosmosStart文件升级
	SYS_SET_IP_ADDR					= (0x00020007),	// 设置系统的IP地址
	SYS_SET_DEVICE_TYPE				= (0x00020008),	// 设置设备类型，RDM_LIGHT = 0x00000000随机光
														// CAM2_LIGHT1 = 0x00000001双相机单光机   
														// CAM1_LIGHT2 = 0x00000002单相机双光机

	// 与应用软件安全性相关的tag
	SYS_ENCRYPT_SYS_INIT			= (0x00021001),	// 使用mac地址和相机sn生成md5序列号，
														//并将序列号写入到 /root/loid文件里
	SYS_ENCRYPT_RESTORE_PWD			= (0x00021002),	// 带正确的key后将系统密码改为简单密码，同时打开屏幕显示开关

	SYS_HEARTBEAT_CFG_SET			= 0x00022001,	// 设置心跳使能/周期 1:enable ms
	SYS_HEARTBEAT_CFG_GET			= 0x00022002,	// 查询心跳使能/周期
	SYS_HEARTBEAT_KICK				= 0x00022003,	// 心跳包 无内容

	SYS_LOG_SERVER_SET				= 0x00023001,	// 设置udp服务器(接收日志)(ip地址:端口号)
	SYS_LOG_SERVER_GET				= 0x00023002,	// 查询udp服务器(接收日志)

	// 设置终端的类型
	SET_DEVTYPE_RL					= (0x00030000),	// 终端类型=随机光
	SET_DEVTYPE_TR					= (0x00030001),	// 终端类型=条纹光

	// 双相机单光机硬触发
	CAMERA_FOR_HTR_PROJ				= (0x00100000),	// 硬触发项目
	CAMERA_GET_HTR22_IMG			= (0x00100001),	// 获取连续22张的标定图片
	CAMERA_GET_HTR12_IMG			= (0x00100002),	// 获取连续12张的识别图片
	CAMERA_GET_HTR34_IMG			= (0x00100003),	// 获取34张图片
	CAMERA_GET_HTR1_IMG				= (0x00100004),	// 可以连续发送常规图像
	CAMERA_GET_HTR37_IMG			= (0x00100005),	// 一般图片1张+三组识别图12*3张
	SET_LIGHT_VALUE					= (0x00100006),	// 设置光机亮度
	GET_LIGHT_VALUE					= (0x00100007),	// 获取当前光机亮度
	CAMERA_GET_HTR1_RGB_IMG			= (0x00100008),	// 获取双目硬触发RGB图像

	// 单相机双光机硬触发
	CAMERA_GET_C1HTR22_IMG			= (0x00110001),	// 获取连续22张的标定图片
	CAMERA_GET_C1HTR12_IMG			= (0x00110002),	// 获取连续12张的识别图片
	CAMERA_GET_C1HTR34_IMG			= (0x00110003),	// 获取34张图片
	CAMERA_GET_C1HTR1_IMG			= (0x00110004),	// 常规图像
	CAMERA_GET_C1HTR37_IMG			= (0x00110005),	// 常规图像

	// 4相机单光机
	CAMERA_GET_C4HTR22_IMG			= (0x00120001),	// 获取连续22张的标定图片
	CAMERA_GET_C4HTR12_IMG			= (0x00120002),	// 获取连续12张的识别图片
	CAMERA_GET_C4HTR34_IMG			= (0x00120003),	// 获取34张图片
	CAMERA_GET_C4HTR1_IMG			= (0x00120004),	// 常规图像
	CAMERA_GET_C4HTR37_IMG			= (0x00120005),	// 常规图像

	CONNECT_ERROR					= (0xFFFFFFFF)	// 连接出错
} csv_cmd_e;

typedef enum {
	RDM_LIGHT						= (0x00000000),	// 随机光
	CAM2_LIGHT1						= (0x00000001),	// 双相机单光机
	CAM1_LIGHT2						= (0x00000002),	// 单相机双光机
	CAM4_LIGHT1						= (0x00000003),	// 四相机单光机
} device_type;





#endif /* __CSV_TYPE_H__ */

