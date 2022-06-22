#ifndef __CSV_MSG_H__
#define __CSV_MSG_H__

#ifdef __cplusplus
extern "C" {
#endif


struct msg_head_t {
	csv_cmd_e			cmdtype;		///< 命令代号
	int					length;			///< 后续数据长度
	int					result;			///< 返回码
};


struct csv_msg_t {


};


extern int csv_msg_init (void);

extern int csv_msg_deinit (void);


#ifdef __cplusplus
}
#endif

#endif /* __CSV_MSG_H__ */


