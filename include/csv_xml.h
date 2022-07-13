#ifndef __CSV_XML_H__
#define __CSV_XML_H__

#ifdef __cplusplus
extern "C" {
#endif

#define FILE_PATH_XML			("csvcfg.xml")	///< 配置xml备份路径
#define FILE_PATH_XML_BK		("../../target/csvcfg.xml")	///< 配置xml文件路径

#define MAX_KEY_VALUE_PAIRS 	32
#define MAX_KEY_SIZE			48

#define XML_GET					(0)		///< 读取 xml 配置
#define XML_SET					(1)		///< 写入 xml 配置

enum {
	XML_VALUE_STRING,
	XML_VALUE_INT8,
	XML_VALUE_UINT8,
	XML_VALUE_INT16,
	XML_VALUE_UINT16,
	XML_VALUE_UINT64,
	XML_VALUE_INT32,
	XML_VALUE_UINT32,
	XML_VALUE_FLOAT,
	XML_VALUE_MAX
};

struct key_value_pair_t {
	void				*value;
	char				key[MAX_KEY_SIZE];
	xmlElementType		nodeType;
	uint8_t				value_type;
};

struct csv_xml_t {
	char				*file;					///< xml 文件绝对路径

	uint8_t				SaveFile;				///< 保存标志

	xmlDocPtr			pDoc;					///< 文件指针
	xmlNodePtr			pRoot;					///< 根指针
	xmlNodePtr			pNode;					///< 1级节点指针
	xmlNodePtr			pNode2;					///< 2级节点指针
	xmlNodePtr			pNode3;					///< 3级节点指针
	xmlNodePtr			pNode4;					///< 4级节点指针
	xmlNodePtr			pNode5;					///< 5级节点指针
	xmlNodePtr			pNode6;					///< 6级节点指针
};


extern int csv_xml_write_DeviceParameters (void);

extern int csv_xml_write_DepthImgParameters (void);

extern int csv_xml_init (void);

extern int csv_xml_deinit (void);


#ifdef __cplusplus
}
#endif


#endif /* __CSV_XML_H__ */

