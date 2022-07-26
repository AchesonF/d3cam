#ifndef __CSV_CFG_H__
#define __CSV_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

struct device_cfg_t {
	int					device_type;
	uint8_t				camera_leftright_type;
	uint8_t				camera_img_type;
	float				exposure_time;
	float				exposure_time_for_rgb;

	float				dlp_rate;
	float				dlp_brightness;

	uint8_t				img_type;		// SUFFIX_BMP
	char				*strSuffix;
};

struct depthimg_cfg_t {
	int					numDisparities;
	int					blockSize;
	int					uniqRatio;
};


/* GigE Version regiseter's parameters */
struct gev_conf_t {
	uint16_t				VersionMajor;
	uint16_t				VersionMinor;
	uint32_t				DeviceMode;	// 
	uint32_t				MacHi;		// mac[0-1]
	uint32_t				MacLow;		// mac[2-5]

	uint32_t				IfCapability0;
	uint32_t				IfConfiguration0;
	uint32_t				CurrentIPAddress0;
	uint32_t				CurrentSubnetMask0;
	uint32_t				CurrentDefaultGateway0;

	char					ManufacturerName[32];
	char					ModelName[32];
	char					DeviceVersion[32];
	char					ManufacturerInfo[48];
	char					SerialNumber[16];
	char					UserdefinedName[16];
	char					FirstURL[GVCP_URL_MAX_LEN];
	char					SecondURL[GVCP_URL_MAX_LEN];
	char					*strXmlfile;
	uint8_t					*xmlData;

	uint32_t				GVSPCapability;
	uint32_t				MessageCapability;
	uint32_t				ActionDeviceKey;
	uint32_t				GVCPCapability;
	uint32_t				HeartbeatTimeout;
	uint32_t				TimestampControl;
	uint32_t				DiscoveryACKDelay;
	uint32_t				GVCPConfiguration;
	uint32_t				ControlSwitchoverKey;
	uint32_t				GVSPConfiguration;
	uint32_t				PhysicalLinkConfiguration;
	uint32_t				ControlChannelPrivilege;

	uint32_t				PrimaryAddress;
	uint16_t				PrimaryPort;

	uint32_t				ChannelAddress0;
	uint16_t				ChannelPort0;
	uint32_t				ChannelPacketSize0;
	uint32_t				ChannelPacketDelay0;
	uint32_t				ChannelConfiguration0;

	uint32_t				ChannelAddress1;
	uint16_t				ChannelPort1;
	uint32_t				ChannelPacketSize1;
	uint32_t				ChannelPacketDelay1;
	uint32_t				ChannelConfiguration1;

	uint32_t				MessageAddress;
	uint16_t				MessagePort;
	uint32_t				MessageTimeout;
	uint32_t				MessageRetryCount;
	uint32_t				MessageSourcePort;

};

struct pointcloud_cfg_t {
	char					imgRoot[128];	///< 相对路径
	char					imgPrefixNameL[128];///< 左图名称前缀
	char					imgPrefixNameR[128];///< 右图名称前缀
	char					outFileXYZ[256];	///< 生成文件名
	uint8_t					enable;				///< 使能计算
};

// 标定
struct calib_conf_t {
	char					path[128];		///< 图片相对路径
	char					calibFile[128];	///< 标定文件名
	uint8_t					groupDemarcate;	///< 标定次数, need to save for next boot

};

struct csv_cfg_t {
	struct device_cfg_t		devicecfg;
	struct depthimg_cfg_t	depthimgcfg;
	struct pointcloud_cfg_t	pointcloudcfg;

	struct calib_conf_t		calibcfg;
	struct gev_conf_t		gigecfg;			///< gev参数
};


extern int csv_cfg_init (void);

extern int csv_cfg_deinit (void);

#ifdef __cplusplus
}
#endif

#endif /* __CSV_CFG_H__ */

