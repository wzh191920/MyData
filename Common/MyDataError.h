#pragma once

#define MYDATA_OK					0x00000000
#define MYDATA_ERROR				-1

#define MYDATA_DATA_NOT_NEW			0xFFFF0101 //插入数据的时间戳不是最新的
#define MYDATA_DATA_TIME_EXIST		0xFFFF0102 //插入数据的时间戳已存在
#define MYDATA_DATA_NOT_EXIST		0xFFFF0103 //数据不存在
#define MYDATA_DATA_TOO_LONG		0xFFFF0104 //数据长度太长
#define MYDATA_DATA_PAR_SUC			0xFFFF0105 //操作部分失败
#define MYDATA_DATA_QUERY_END		0xFFFF0106 //读取数据结束

#define MYDATA_NO_MEM				0xFFFF0201 //内存分配失败
#define MYDATA_NET_TIMEOUT			0xFFFF0202 //网络超时
#define MYDATA_RECV_FAIL			0xFFFF0203 //网络接收失败
#define MYDATA_CRC_FAIL				0xFFFF0204 //CRC校验失败
#define MYDATA_ERROR_TYPE			0xFFFF0205 //消息类型错误
#define MYDATA_MSG_SER_FAIL			0xFFFF0206 //序列化失败
#define MYDATA_SEND_FAIL			0xFFFF0207 //网络发送失败
#define MYDATA_INVALIDE_HANDLE		0xFFFF0208 //无效的句柄
#define MYDATA_HANDLE_CONFLICT		0xFFFF0209 //句柄使用冲突
#define MYDATA_INVALIDE_PARA		0xFFFF0210 //无效的参数
#define MYDATA_CONNECT_FAIL			0xFFFF0211 //网络连接失败

#define MYDATA_SQL_TYPE_FAIL 		0xFFFF0301 //操作类型失败
#define MYDATA_SQL_TABLE_FAIL 		0xFFFF0302 //操作表失败
#define MYDATA_SQL_TABLE_NOT_ALLOWD 0xFFFF0303 //有相关表存在，不能执行操作
#define MYDATA_SQL_POINT_FAIL 		0xFFFF0304 //操作标签点失败
#define MYDATA_SQL_POINT_NOT_ALLOWD	0xFFFF0305 //有相关标签点存在，不能执行操作
#define MYDATA_SQL_POINT_END		0xFFFF0306 //标签点查询结束

#define MYDATA_CREATEFILE_FAIL		0xFFFF0401 //数据文件创建失败
#define MYDATA_DELETEFILE_FAIL		0xFFFF0402 //数据文件删除失败
#define MYDATA_FILEINIT_FAIL		0xFFFF0403 //文件映射初始化失败
#define MYDATA_NO_EMPTY_FILE		0xFFFF0404 //没有空历史文件
#define MYDATA_NO_FIT_FILE			0xFFFF0405 //没有合适的历史文件
#define MYDATA_INVALIDE_FILENAME	0xFFFF0406 //历史文件名不合法
#define MYDATA_FILE_CONFLICT		0xFFFF0407 //历史文件冲突
#define MYDATA_REBUILD_INDEX_FAIL	0xFFFF0408 //重建索引失败

#define MYDATA_POINT_NOT_EXIST		0xFFFF0501 //指定的标签点不存在
#define MYDATA_SERIAL_FAIL			0xFFFF0502 //序列化或反序列化失败
#define MYDATA_NAME_TOOLONG			0xFFFF0503 //名称长度超过限制

#define MYDATA_PLUGIN_PREVENT		0xFFFF0601 //插件阻止操作