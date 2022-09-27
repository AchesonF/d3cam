#ifndef __CSV_MSG_H__
#define __CSV_MSG_H__

#ifdef __cplusplus
extern "C" {
#endif


#define NAME_THREAD_MSG					"'thr_msg'"


/* 消息头结构 */
struct msg_head_t {
	csv_cmd_e				cmdtype;		///< 命令代号
	int						length;			///< 后续数据长度
	int						result;			///< 返回码
};

/* 消息数据整包 */
struct msg_package_t {
	struct msg_head_t		hdr;
	uint8_t					*payload;
};

/* 消息列表 */
struct msglist_t {
	struct msg_package_t	mp;
	struct list_head		list;
};

/* tcp 应答原始数据 */
struct msg_ack_t {
	uint32_t				len_send;
	uint8_t					*buf_send;
};

/* 单个命令结构 */
struct msg_command_t {
	csv_cmd_e				cmdtype;
	char					*cmdname;
	int 					(*func)(struct msg_package_t *pMP, struct msg_ack_t *pACK);
};

/* 命令列表 */
struct msg_command_list {
	struct msg_command_t	command;
	struct list_head		list;
};


/* 消息处理结构 */
struct csv_msg_t {

	struct msg_command_list	head_cmd;		///< 命令注册链表
	struct msglist_t		head_msg;		///< 消息链表

	struct msg_ack_t		ack;			///< 应答数据

	const char				*name_msg;		///< 消息
	pthread_t				thr_msg;		///< ID
	pthread_mutex_t			mutex_msg;		///< 锁
	pthread_cond_t			cond_msg;		///< 条件
};

extern int csv_msg_ack_package (struct msg_package_t *pMP, struct msg_ack_t *pACK, 
	char *content, int len, int retcode);

extern int csv_msg_send (struct msg_ack_t *pACK);

extern void msg_command_add (csv_cmd_e cmdtype, char *cmdname, 
	int (*func)(struct msg_package_t *pMP, struct msg_ack_t *pACK));

extern void csv_general_msg_cmd_enroll (void);

extern void csv_hk_msg_cmd_enroll (void);

extern void csv_gx_msg_cmd_enroll (void);

extern int csv_msg_check (uint8_t *buf, uint32_t len);

extern int csv_msg_init (void);

extern int csv_msg_deinit (void);


#ifdef __cplusplus
}
#endif

#endif /* __CSV_MSG_H__ */


