// MyDataAPI.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <memory>
#include <algorithm>
#include "ace/SOCK_Connector.h"
#include "ace/Time_Value.h"
#include "ClientProtoHandler.h"
#include "MyDataAPI.h"
#include "MyDataError.h"
#include "MyDataCommon.h"
#include "ApiCommon.h"
#include "net.pb.h"
#include "points.pb.h"
#include "datas.pb.h"
#include "MessageCommon.h"
using std::unique_ptr;

extern map<int, TypeInfo> _types;
int MyDataConnect(const char *ip, int port, int *handle)
{
	if (ip == NULL || handle == NULL || strlen(ip) > IP_LENGTH-1)
		return MYDATA_INVALIDE_PARA;
	ClientInfo *cinfo = new ClientInfo(ip, port);
	unique_ptr<ClientInfo> ptr(cinfo);
	ACE_SOCK_Stream &sock = cinfo->GetSocket();
	ACE_SOCK_Connector con;
	ACE_INET_Addr addr(port, ip);
	ACE_Time_Value timeout(5, 0);
	int ret = con.connect(sock, addr, &timeout, ACE_Addr::sap_any, 1);
	if(ret)
	{
		return MYDATA_CONNECT_FAIL;
	}

	int snd_size = 1024 * 1024 * 2;
	int rec_size = 1024 * 1024 * 2;
	socklen_t optlen = sizeof(snd_size);
	sock.set_option(SOL_SOCKET, SO_SNDBUF, (char*)&snd_size, optlen);
	sock.set_option(SOL_SOCKET, SO_RCVBUF, (char*)&rec_size, optlen);
	proto_handler ph(cinfo->GetRoot());
	MydataNet::ConnectQuery connect_query;
	connect_query.set_unused(0);
	ret = ph.send_all(sock, connect_query, MsgConnectQuery);
	if (ret)
	{
		return ret;
	}
	Common::Head head;
	ret = ph.recv_head(sock, MsgConnectReply, head);
	if (ret)
	{
		return ret;
	}

	MydataNet::ConnectReply connect_reply;
	ret = ph.recv_body(sock, head, connect_reply);
	if (ret)
	{
		return ret;
	}
	ret = connect_reply.err();
	if (ret != MYDATA_OK)
		return ret;
	ptr.release();
	*handle = cinfo->GetHandle();
	cinfo->AddClientInfo();
	cinfo->IsConnect(true);
	return ret;
}

int MyDataDisconnect(int handle)
{
	ClientInfo *cinfo = NULL;
	int ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;
	{
		ClientScopeLock(cinfo);
		cinfo->RemoveClientInfo();
	}
	delete cinfo;
	return MYDATA_OK;
}

void MyDataTimeout(int sec)
{
	proto_handler::_timeout.set(sec, 0);
}

const char* MyDataGetErrDesc(int err)
{
	struct ErrInfo
	{
		unsigned int code;
		const char* msg;
	};
	static ErrInfo err_info[] =
	{
		{0x00000000,		"操作成功"},

		{0xFFFF0101,		"插入数据的时间戳不是最新的"},
		{0xFFFF0102,		"插入数据的时间戳已存在"},
		{0xFFFF0103,		"数据不存在"},
		{0xFFFF0104,		"数据长度太长"},
		{0xFFFF0105,		"操作部分失败"},
		{0xFFFF0106,		"读取数据结束"},

		{0xFFFF0201,		"内存分配失败"},
		{0xFFFF0202,		"网络超时"},
		{0xFFFF0203,		"网络接收失败"},
		{0xFFFF0204,		"CRC校验失败"},
		{0xFFFF0205,		"消息类型错误"},
		{0xFFFF0206,		"序列化失败"},
		{0xFFFF0207,		"网络发送失败"},
		{0xFFFF0208,		"无效的句柄"},
		{0xFFFF0209,		"句柄使用冲突"},
		{0xFFFF0210,		"无效的参数"},
		{0xFFFF0211,		"网络连接失败"},

		{0xFFFF0301,		"操作类型失败"},
		{0xFFFF0302,		"操作表失败"},
		{0xFFFF0303,		"有相关表存在，不能执行操作"},
		{0xFFFF0304,		"操作标签点失败"},
		{0xFFFF0305,		"有相关标签点存在，不能执行操作"},
		{0xFFFF0306,		"标签点查询结束"},

		{0xFFFF0401,		"数据文件创建失败"},
		{0xFFFF0402,		"数据文件删除失败"},
		{0xFFFF0403,		"文件映射初始化失败"},
		{0xFFFF0404,		"没有空历史文件"},
		{0xFFFF0405,		"没有合适的历史文件"},
		{0xFFFF0406,		"历史文件名不合法"},
		{0xFFFF0407,		"历史文件冲突"},
		{0xFFFF0408,		"重建索引失败"},

		{0xFFFF0501,		"指定的标签点不存在"},
		{0xFFFF0502,		"序列化或反序列化失败"},
		{0xFFFF0503,		"名称长度超过限制"},

		{0xFFFF0601,		"插件阻止操作"},

		{-1,				"操作失败"},

	};
	ErrInfo found;
	found.code = err;
	static int end = sizeof(err_info)/sizeof(ErrInfo);
	ErrInfo *perr_info = err_info;
	ErrInfo *found_err = std::lower_bound(perr_info, perr_info+end, found, [](const ErrInfo &e1, const ErrInfo &e2){
		return e1.code < e2.code;
	});
	if (found_err == perr_info+end || found_err->code != err)
		return "未知错误";
	return found_err->msg;
}

int MyDataAddType(int handle, DataType* type_array, int array_size, const char* name, const char* type_names)
{
	if (type_array == NULL || array_size <=0 || array_size > TYPE_SIZE || name == NULL || strlen(name) == 0 || type_names == NULL)
		return MYDATA_INVALIDE_PARA;
	if (strlen(name)>=NAME_SIZE || strlen(type_names)>=512)
		return MYDATA_NAME_TOOLONG;
	if (array_size > std::count(type_names, type_names + strlen(type_names), ';') + 1)
		return MYDATA_INVALIDE_PARA;
	char carray[17] = {0};
	for (int i=0; i<array_size; i++)
	{
		if (type_array[i] < DataChar || type_array[i] > DataString)
			return MYDATA_INVALIDE_PARA;
		carray[i] = type_array[i];
	}

	ClientInfo *cinfo = NULL;
	int ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;
	
	ClientScopeLock(cinfo);
	cln_proto_handler client_handler(cinfo->GetRoot());
	Points::AddTypeQuery query;
	query.set_type(carray);
	query.set_name(name);
	query.set_type_names(type_names);
	ret = client_handler.send_all(*cinfo, query, MsgAddTypeQuery);
	if (ret)
		return ret;
	
	Common::Head head;
	Points::AddTypeReply reply;
	ret = client_handler.recv_all(*cinfo, MsgAddTypeReply, head, reply);
	if (ret)
		return ret;
	ret = reply.err();
	return ret;
}

int MyDataDeleteType(int handle, int type_id)
{
	ClientInfo *cinfo = NULL;
	int ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;

	ClientScopeLock(cinfo);
	cln_proto_handler client_handler(cinfo->GetRoot());
	Points::DeleteTypeQuery query;
	query.set_id(type_id);
	ret = client_handler.send_all(*cinfo, query, MsgDeleteTypeQuery);
	if (ret)
		return ret;

	Common::Head head;
	Points::DeleteTypeReply reply;
	ret = client_handler.recv_all(*cinfo, MsgDeleteTypeReply, head, reply);
	if (ret)
		return ret;
	ret = reply.err();
	return ret;
}

int MyDataGetTypeNum(int handle, int *num)
{
	if (num == NULL)
		return MYDATA_INVALIDE_PARA;
	ClientInfo *cinfo = NULL;
	int ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;

	ClientScopeLock(cinfo);
	cln_proto_handler client_handler(cinfo->GetRoot());
	Points::GetTypesNumQuery query;
	query.set_unused(0);
	ret = client_handler.send_all(*cinfo, query, MsgGetTypesNumQuery);
	if (ret)
		return ret;

	Common::Head head;
	Points::GetTypesNumReply reply;
	ret = client_handler.recv_all(*cinfo, MsgGetTypesNumReply, head, reply);
	if (ret)
		return ret;
	ret = reply.err();
	if (ret)
		return ret;

	*num = reply.num();
	return ret;
}

int MyDataGetTypes(int handle, TypeInfo *type_infos, int *num)
{	
	if (type_infos == NULL || num == NULL || *num <= 0)
		return MYDATA_INVALIDE_PARA;
	ClientInfo *cinfo = NULL;
	int ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;

	ClientScopeLock(cinfo);
	cln_proto_handler client_handler(cinfo->GetRoot());
	Points::GetTypesQuery query;
	query.set_num(*num);
	ret = client_handler.send_all(*cinfo, query, MsgGetTypesQuery);
	if (ret)
		return ret;

	Common::Head head;
	Points::GetTypesReply reply;
	ret = client_handler.recv_all(*cinfo, MsgGetTypesReply, head, reply);
	if (ret)
		return ret;
	ret = reply.err();
	if (ret)
		return ret;

	auto array = reply.type_info_array();
	int i = 0;
	for (auto iter = array.begin(); iter != array.end() && i < *num; ++iter, ++i)
	{
		SafeCopy(iter->name(), type_infos[i].name, NAME_SIZE);
		SafeCopy(iter->type_names(), type_infos[i].type_names, sizeof(type_infos[i].type_names));
		memcpy(type_infos[i].type, iter->type().c_str(), iter->type().length());
		if (iter->type().length() < TYPE_SIZE)
			type_infos[i].type[iter->type().length()] = 0;
		type_infos[i].id = iter->id();
	}
	*num = i;
	return ret;
}

MYDATA_API int MyDataGetTypeByID(int handle, int type_id, TypeInfo *type_info)
{
	if (type_info == NULL)
		return MYDATA_INVALIDE_PARA;

	ClientInfo *cinfo = NULL;
	int ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;

	ClientScopeLock(cinfo);
	cln_proto_handler client_handler(cinfo->GetRoot());
	Points::GetTypeByIDQuery query;
	query.set_id(type_id);
	ret = client_handler.send_all(*cinfo, query, MsgGetTypeByIDQuery);
	if (ret)
		return ret;

	Common::Head head;
	Points::GetTypeByIDReply reply;
	ret = client_handler.recv_all(*cinfo, MsgGetTypeByIDReply, head, reply);
	if (ret)
		return ret;
	ret = reply.err();
	if (ret)
		return ret;
	const Points::TypeInfo &info = reply.info();
	type_info->id = info.id();
	SafeCopy(info.name(), type_info->name, sizeof(type_info->name));
	SafeCopy(info.type_names(), type_info->type_names, sizeof(type_info->type_names));
	memset(type_info->type, 0, sizeof(type_info->type));
	memcpy(type_info->type, info.type().c_str(), info.type().length());
	return ret;
}

MYDATA_API int MyDataAddTable(int handle, int type_id, const char *name)
{
	if (name == NULL || strlen(name) == 0)
		return MYDATA_INVALIDE_PARA;
	if (strlen(name) >= NAME_SIZE)
		return MYDATA_NAME_TOOLONG;

	ClientInfo *cinfo = NULL;
	int ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;

	ClientScopeLock(cinfo);
	cln_proto_handler client_handler(cinfo->GetRoot());
	Points::AddTableQuery query;
	query.set_type_id(type_id);
	query.set_name(name);
	ret = client_handler.send_all(*cinfo, query, MsgAddTableQuery);
	if (ret)
		return ret;

	Common::Head head;
	Points::AddTableReply reply;
	ret = client_handler.recv_all(*cinfo, MsgAddTableReply, head, reply);
	if (ret)
		return ret;
	ret = reply.err();
	return ret;
}

MYDATA_API int MyDataDeleteTable(int handle, int table_id)
{
	ClientInfo *cinfo = NULL;
	int ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;

	ClientScopeLock(cinfo);
	cln_proto_handler client_handler(cinfo->GetRoot());
	Points::DeleteTableQuery query;
	query.set_id(table_id);
	ret = client_handler.send_all(*cinfo, query, MsgDeleteTableQuery);
	if (ret)
		return ret;

	Common::Head head;
	Points::DeleteTableReply reply;
	ret = client_handler.recv_all(*cinfo, MsgDeleteTableReply, head, reply);
	if (ret)
		return ret;
	ret = reply.err();
	return ret;
}

MYDATA_API int MyDataGetTableNum(int handle, int *num)
{
	if (num == NULL)
		return MYDATA_INVALIDE_PARA;
	ClientInfo *cinfo = NULL;
	int ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;

	ClientScopeLock(cinfo);
	cln_proto_handler client_handler(cinfo->GetRoot());
	Points::GetTablesNumQuery query;
	query.set_unused(0);
	ret = client_handler.send_all(*cinfo, query, MsgGetTablesNumQuery);
	if (ret)
		return ret;

	Common::Head head;
	Points::GetTablesNumReply reply;
	ret = client_handler.recv_all(*cinfo, MsgGetTablesNumReply, head, reply);
	if (ret)
		return ret;
	ret = reply.err();
	if (ret)
		return ret;
	*num = reply.num();
	return ret;
}

MYDATA_API int MyDataGetTables(int handle, TableInfo *table_infos, int *num)
{
	if (table_infos == NULL || num == NULL || *num <= 0)
		return MYDATA_INVALIDE_PARA;
	ClientInfo *cinfo = NULL;
	int ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;

	ClientScopeLock(cinfo);
	cln_proto_handler client_handler(cinfo->GetRoot());
	Points::GetTablesQuery query;
	query.set_num(*num);
	ret = client_handler.send_all(*cinfo, query, MsgGetTablesQuery);
	if (ret)
		return ret;

	Common::Head head;
	Points::GetTablesReply reply;
	ret = client_handler.recv_all(*cinfo, MsgGetTablesReply, head, reply);
	if (ret)
		return ret;
	ret = reply.err();
	if (ret)
		return ret;

	auto array = reply.table_info_array();
	int i = 0;
	for (auto iter = array.begin(); iter != array.end() && i < *num; ++iter, ++i)
	{
		SafeCopy(iter->name(), table_infos[i].name, NAME_SIZE);
		table_infos[i].id = iter->id();
		table_infos[i].type_id = iter->type_id();
	}
	*num = i;
	return ret;
}

MYDATA_API int MyDataGetTableByID(int handle, TableInfo *table_info, int table_id)
{
	if (table_info == NULL)
		return MYDATA_INVALIDE_PARA;
	ClientInfo *cinfo = NULL;
	int ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;

	ClientScopeLock(cinfo);
	cln_proto_handler client_handler(cinfo->GetRoot());
	Points::GetTableByIDQuery query;
	query.set_id(table_id);
	ret = client_handler.send_all(*cinfo, query, MsgGetTableByIDQuery);
	if (ret)
		return ret;

	Common::Head head;
	Points::GetTableByIDReply reply;
	ret = client_handler.recv_all(*cinfo, MsgGetTableByIDReply, head, reply);
	if (ret)
		return ret;
	ret = reply.err();
	if (ret)
		return ret;
	table_info->id = reply.id();
	table_info->type_id = reply.type_id();
	SafeCopy(reply.name(), table_info->name, sizeof(table_info->name));
	return ret;
}

MYDATA_API int MyDataAddPoints(int handle, int table_id, char **point_names, int *num)
{
	if (point_names == NULL || num == NULL)
		return MYDATA_INVALIDE_PARA;

	ClientInfo *cinfo = NULL;
	int ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;

	ClientScopeLock(cinfo);
	cln_proto_handler client_handler(cinfo->GetRoot());
	Points::AddPointsQuery query;
	auto points = query.mutable_point_names();
	points->Reserve(*num);
	for (int i=0; i<*num; i++)
	{
		string* point_name = points->Add();
		*point_name = point_names[i];
		if (point_name->length() >= NAME_SIZE)
			return MYDATA_NAME_TOOLONG;
	}
	query.set_table_id(table_id);
	ret = client_handler.send_all(*cinfo, query, MsgAddPointQuery);
	if (ret)
		return ret;

	Common::Head head;
	Points::AddPointsReply reply;
	ret = client_handler.recv_all(*cinfo, MsgAddPointReply, head, reply);
	if (ret)
		return ret;
	ret = reply.err();
	*num = reply.suc_num();
	return ret;
}

MYDATA_API int MyDataDeletePoint(int handle, int point_id)
{
	ClientInfo *cinfo = NULL;
	int ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;

	ClientScopeLock(cinfo);
	cln_proto_handler client_handler(cinfo->GetRoot());
	Points::DeletePointQuery query;
	query.set_id(point_id);
	ret = client_handler.send_all(*cinfo, query, MsgDeletePointQuery);
	if (ret)
		return ret;

	Common::Head head;
	Points::DeletePointReply reply;
	ret = client_handler.recv_all(*cinfo, MsgDeletePointReply, head, reply);
	if (ret)
		return ret;
	ret = reply.err();
	return ret;
}

MYDATA_API int MyDataSearchPointsNum(int handle, const char * table_name, const char *point_name, int *num)
{
	if (num == NULL || point_name == NULL || table_name == NULL || strlen(point_name) >= NAME_SIZE || strlen(table_name) >= NAME_SIZE)
		return MYDATA_INVALIDE_PARA;
	
	ClientInfo *cinfo = NULL;
	int ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;

	ClientScopeLock(cinfo);
	cln_proto_handler client_handler(cinfo->GetRoot());
	Points::SearchPointsNumQuery query;
	char namebuf[NAME_SIZE];
	if (strlen(table_name) == 0)
		query.set_table_name("%");
	else 
		query.set_table_name(StrReplace(table_name, namebuf, '*', '%'));
	if (strlen(point_name) == 0)
		query.set_point_name("%");
	else
		query.set_point_name(StrReplace(point_name, namebuf, '*', '%'));

	ret = client_handler.send_all(*cinfo, query, MsgSearchPointsNumQuery);
	if (ret)
		return ret;

	Common::Head head;
	Points::SearchPointsNumReply reply;
	ret = client_handler.recv_all(*cinfo, MsgSearchPointsNumReply, head, reply);
	if (ret)
		return ret;
	ret = reply.err();
	if (ret)
		return ret;
	*num = reply.num();
	return ret;
}

MYDATA_API int MyDataSearchPointsNumByID(int handle, int table_id, const char *point_name, int *num)
{
	if (num == NULL || point_name == NULL || strlen(point_name) >= NAME_SIZE)
		return MYDATA_INVALIDE_PARA;

	ClientInfo *cinfo = NULL;
	int ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;

	ClientScopeLock(cinfo);
	cln_proto_handler client_handler(cinfo->GetRoot());
	Points::SearchPointsNumByIDQuery query;
	query.set_table_id(table_id);
	char namebuf[NAME_SIZE];
	if (strlen(point_name) == 0)
		query.set_point_name("%");
	else
		query.set_point_name(StrReplace(point_name, namebuf, '*', '%'));

	ret = client_handler.send_all(*cinfo, query, MsgSearchPointsNumBYIDQuery);
	if (ret)
		return ret;

	Common::Head head;
	Points::SearchPointsNumReply reply;
	ret = client_handler.recv_all(*cinfo, MsgSearchPointsNumReply, head, reply);
	if (ret)
		return ret;
	ret = reply.err();
	if (ret)
		return ret;
	*num = reply.num();
	return ret;
}

MYDATA_API int MyDataSearchPoints(int handle, const char * table_name, const char *point_name, PointInfo *point_infos, int *num)
{
	if (point_infos == NULL || num == NULL || *num <= 0 || point_name == NULL || table_name == NULL || strlen(point_name) >= NAME_SIZE || strlen(table_name) >= NAME_SIZE)
		return MYDATA_INVALIDE_PARA;
	ClientInfo *cinfo = NULL;
	int ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;
	ClientScopeLock(cinfo);
	char namebuf[NAME_SIZE];
	cln_proto_handler client_handler(cinfo->GetRoot());
	Points::SearchPointsQuery query;
	if (strlen(table_name) == 0)
		query.set_table_name("%");
	else
		query.set_table_name(table_name);
	if (strlen(point_name) == 0)
		query.set_point_name("%");
	else
		query.set_point_name(point_name);
	query.set_from_id(0);
	int success_num = 0;
	int i = 0;
	while (ret == 0)
	{
		ret = client_handler.send_all(*cinfo, query, MsgSearchPointsQuery);
		if (ret)
			break;

		Common::Head head;
		Points::SearchPointsReply reply;
		ret = client_handler.recv_all(*cinfo, MsgSearchPointsReply, head, reply);
		if (ret)
			break;

		auto array = reply.point_info_array();
		
		for (auto iter = array.begin(); iter != array.end() && i< *num; ++iter, ++i)
		{
			SafeCopy(iter->name(), point_infos[i].name, NAME_SIZE);
			point_infos[i].id = iter->id();
			point_infos[i].table_id = iter->table_id();
		}
		query.set_from_id(point_infos[i-1].id);
		ret = reply.err();
	}
	*num = i;
	if (ret == MYDATA_SQL_POINT_END)
		ret = MYDATA_OK;
	return ret;
}

MYDATA_API int MyDataSearchPointsByID(int handle, int table_id, const char *point_name, PointInfo *point_infos, int *num)
{
	if (point_infos == NULL || num == NULL || *num <= 0 || point_name == NULL || strlen(point_name) >= NAME_SIZE)
		return MYDATA_INVALIDE_PARA;

	ClientInfo *cinfo = NULL;
	int ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;
	ClientScopeLock(cinfo);
	char namebuf[NAME_SIZE];
	cln_proto_handler client_handler(cinfo->GetRoot());
	Points::SearchPointsByIDQuery query;
	query.set_table_id(table_id);
	if (strlen(point_name) == 0)
		query.set_point_name("%");
	else
		query.set_point_name(point_name);
	query.set_from_id(0);
	int i = 0;
	while (ret == 0)
	{
		ret = client_handler.send_all(*cinfo, query, MsgSearchPointsByIDQuery);
		if (ret)
			break;

		Common::Head head;
		Points::SearchPointsReply reply;
		ret = client_handler.recv_all(*cinfo, MsgSearchPointsReply, head, reply);
		if (ret)
			break;

		auto array = reply.point_info_array();

		for (auto iter = array.begin(); iter != array.end() && i< *num; ++iter, ++i)
		{
			SafeCopy(iter->name(), point_infos[i].name, NAME_SIZE);
			point_infos[i].id = iter->id();
			point_infos[i].table_id = iter->table_id();
		}
		query.set_from_id(point_infos[i-1].id);
		ret = reply.err();
	}
	*num = i;
	if (ret == MYDATA_SQL_POINT_END)
		ret = MYDATA_OK;
	return ret;
}

MYDATA_API int MakeSerializer(int handle, int type_id, bool sync, Serializer *io)
{
	int ret = 0;
	if (sync)
	{
		TypeInfo info;
		ret = MyDataGetTypeByID(handle, type_id, &info);
		if (ret)
			return ret;
		_types[type_id] = info;
		memcpy(io->type, info.type, sizeof(info.type));
		io->offset = 0;
		io->data_pos = 0;
		return ret;
	}
	map<int, TypeInfo>::iterator iter = _types.find(type_id);
	if (iter != _types.end())
	{
		memcpy(io->type, iter->second.type, sizeof(iter->second.type));
		io->offset = 0;
		io->data_pos = 0;
		return ret;
	}
	else
	{
		TypeInfo info;
		ret = MyDataGetTypeByID(handle, type_id, &info);
		if (ret)
			return ret;
		_types[type_id] = info;
		memcpy(io->type, info.type, sizeof(info.type));
		io->offset = 0;
		io->data_pos = 0;
		return ret;
	}
}

MYDATA_API int WriteChar(Serializer* io, char data)
{
	if (io->type[io->offset] != DataChar)
	{
		return MYDATA_SERIAL_FAIL;
	}
	io->buf[io->data_pos++] = data;
	io->offset++;
	return MYDATA_OK;
}

MYDATA_API int WriteShort(Serializer* io, short data)
{
	if (io->type[io->offset] != DataInt16)
	{
		return MYDATA_SERIAL_FAIL;
	}
	memcpy(io->buf+io->data_pos, &data, sizeof(data));
	io->data_pos += sizeof(data);
	io->offset++;
	return MYDATA_OK;
}

MYDATA_API int WriteInt(Serializer* io, int data)
{
	if (io->type[io->offset] != DataInt32)
	{
		return MYDATA_SERIAL_FAIL;
	}
	memcpy(io->buf+io->data_pos, &data, sizeof(data));
	io->data_pos += sizeof(data);
	io->offset++;
	return MYDATA_OK;
}

MYDATA_API int WriteLongLong(Serializer* io, long long data)
{
	if (io->type[io->offset] != DataInt64)
	{
		return MYDATA_SERIAL_FAIL;
	}
	memcpy(io->buf+io->data_pos, &data, sizeof(data));
	io->data_pos += sizeof(data);
	io->offset++;
	return MYDATA_OK;
}

MYDATA_API int WriteFloat(Serializer* io, float data)
{
	if (io->type[io->offset] != DataFloat)
	{
		return MYDATA_SERIAL_FAIL;
	}
	memcpy(io->buf+io->data_pos, &data, sizeof(data));
	io->data_pos += sizeof(data);
	io->offset++;
	return MYDATA_OK;
}

MYDATA_API int WriteDouble(Serializer* io, double data)
{
	if (io->type[io->offset] != DataDouble)
	{
		return MYDATA_SERIAL_FAIL;
	}
	memcpy(io->buf+io->data_pos, &data, sizeof(data));
	io->data_pos += sizeof(data);
	io->offset++;
	return MYDATA_OK;
}

MYDATA_API int WriteString(Serializer* io, char* data)
{
	if (io->type[io->offset] != DataString)
	{
		return MYDATA_SERIAL_FAIL;
	}
	int length = strlen(data)+1;
	memcpy(io->buf+io->data_pos, data, length);
	io->data_pos += length;
	io->offset++;
	return MYDATA_OK;
}

MYDATA_API int ReadChar(Serializer* io, char *data)
{
	if (io->type[io->offset] != DataChar)
	{
		return MYDATA_SERIAL_FAIL;
	}
	*data = io->buf[io->data_pos++];
	io->offset++;
	return MYDATA_OK;
}

MYDATA_API int ReadShort(Serializer* io, short *data)
{
	if (io->type[io->offset] != DataInt16)
	{
		return MYDATA_SERIAL_FAIL;
	}
	memcpy(data, io->buf+io->data_pos, sizeof(*data));
	io->data_pos += sizeof(*data);
	io->offset++;
	return MYDATA_OK;
}

MYDATA_API int ReadInt(Serializer* io, int *data)
{
	if (io->type[io->offset] != DataInt32)
	{
		return MYDATA_SERIAL_FAIL;
	}
	memcpy(data, io->buf+io->data_pos, sizeof(*data));
	io->data_pos += sizeof(*data);
	io->offset++;
	return MYDATA_OK;
}

MYDATA_API int ReadLongLong(Serializer* io, long long *data)
{
	if (io->type[io->offset] != DataInt64)
	{
		return MYDATA_SERIAL_FAIL;
	}
	memcpy(data, io->buf+io->data_pos, sizeof(*data));
	io->data_pos += sizeof(*data);
	io->offset++;
	return MYDATA_OK;
}

MYDATA_API int ReadFloat(Serializer* io, float *data)
{
	if (io->type[io->offset] != DataFloat)
	{
		return MYDATA_SERIAL_FAIL;
	}
	memcpy(data, io->buf+io->data_pos, sizeof(*data));
	io->data_pos += sizeof(*data);
	io->offset++;
	return MYDATA_OK;
}

MYDATA_API int ReadDouble(Serializer* io, double *data)
{
	if (io->type[io->offset] != DataDouble)
	{
		return MYDATA_SERIAL_FAIL;
	}
	memcpy(data, io->buf+io->data_pos, sizeof(*data));
	io->data_pos += sizeof(*data);
	io->offset++;
	return MYDATA_OK;
}

MYDATA_API int ReadString(Serializer* io, char* data)
{
	if (io->type[io->offset] != DataString)
	{
		return MYDATA_SERIAL_FAIL;
	}
	int length = strlen(io->buf+io->data_pos)+1;
	memcpy(data, io->buf+io->data_pos, length);
	io->data_pos += length;
	io->offset++;
	return MYDATA_OK;
}

MYDATA_API const void* GetSerializeBuf(const Serializer* io, int *length)
{
	if (io->offset == TYPE_SIZE || io->type[io->offset + 1] == 0)
	{
		*length = io->data_pos;
		return io->buf;
	}
	return NULL;
}

MYDATA_API int WriteNewDatas(int handle, const int *point_ids, const int *table_ids, const time_t *times, const Serializer *datas, int *num)
{
	if (point_ids == NULL || times == NULL || datas == NULL || num == NULL || *num < 1)
		return MYDATA_INVALIDE_PARA;

	ClientInfo *cinfo = NULL;
	int ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;

	ClientScopeLock(cinfo);
	cln_proto_handler client_handler(cinfo->GetRoot());
	Datas::WriteNewDatasQuery &query = cinfo->write_new_query_;
	auto data_infos = query.mutable_data_infos();
	data_infos->Reserve(*num);
	int length = 0;
	const void *data = 0;
	bool is_partial_success = false;
	for (int i=0; i<*num; i++)
	{
		data = GetSerializeBuf(datas+i, &length);
		if (data == NULL)
		{
			is_partial_success = true;
			continue;
		}
		auto data_info = data_infos->Add();
		data_info->set_id(point_ids[i]);
		data_info->set_table_id(table_ids[i]);
		data_info->set_time_stamp(times[i]);
		data_info->set_data(data, length);
	}
	ret = client_handler.send_all(*cinfo, query, MsgWriteNewDatasQuery);
	query.Clear();
	if (ret)
		return ret;

	Common::Head head;
	Datas::WriteNewDatasReply reply;
	ret = client_handler.recv_all(*cinfo, MsgWriteNewDatasReply, head, reply);
	if (ret)
		return ret;
	ret = reply.err();
	*num = reply.num();
	if (ret == 0 && is_partial_success)
		ret = MYDATA_DATA_PAR_SUC;
	return ret;
}

MYDATA_API int ReadNewDatas(int handle, const int *point_ids, const int *type_ids, time_t *times, Serializer *datas, int *num)
{
	if (point_ids == NULL || times == NULL || datas == NULL || num == NULL || *num < 1)
		return MYDATA_INVALIDE_PARA;
	int ret;
	for (int i=0; i<*num; i++)
	{
		ret = MakeSerializer(handle, type_ids[i], false, &datas[i]);
		if (ret)
			return ret;
	}
	ClientInfo *cinfo = NULL;
	ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;
	
	ClientScopeLock(cinfo);
	cln_proto_handler client_handler(cinfo->GetRoot());
	Datas::ReadNewDatasQuery &query = cinfo->read_new_query_;
	auto ids_array = query.mutable_ids();
	ids_array->Reserve(*num);
	for (int i =0; i<*num; i++)
	{
		ids_array->Add(point_ids[i]);
	}
	ret = client_handler.send_all(*cinfo, query, MsgReadNewDatasQuery);
	query.Clear();
	if (ret)
		return ret;

	Common::Head head;
	Datas::ReadNewDatasReply &reply = cinfo->read_new_reply_;
	ret = client_handler.recv_all(*cinfo, MsgReadNewDatasReply, head, reply);
	if (ret)
	{
		reply.Clear();
		return ret;
	}
	*num = reply.num();
	ret = reply.err();
	if (ret != 0 && ret != MYDATA_DATA_PAR_SUC)
	{
		reply.Clear();
		return ret;
	}
	auto array = reply.data_infos();
	int i = 0;
	for (auto iter = array.begin(); iter != array.end(); ++iter, ++i)
	{
		times[i] = iter->time_stamp();
		memcpy(datas[i].buf, iter->data().c_str(), iter->data().length());
	}
	reply.Clear();
	return ret;
}

MYDATA_API int ReadDatasNum(int handle, const int point_id, const time_t time_begin, const time_t time_end, int *num)
{
	if (num == NULL || time_begin > time_end)
		return MYDATA_INVALIDE_PARA;
	ClientInfo *cinfo = NULL;
	int ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;
	ClientScopeLock(cinfo);
	cln_proto_handler client_handler(cinfo->GetRoot());
	Datas::ReadDatasNumQuery query;
	query.set_id(point_id);
	query.set_time_begin(time_begin);
	query.set_time_end(time_end);
	ret = client_handler.send_all(*cinfo, query, MsgReadDatasNumQuery);
	if (ret)
		return ret;

	Common::Head head;
	Datas::ReadDatasNumReply reply;
	ret = client_handler.recv_all(*cinfo, MsgReadDatasNumReply, head, reply);
	if (ret)
		return ret;
	ret = reply.err();
	if (ret)
		return ret;
	*num = reply.num();
	return ret;
}

MYDATA_API int ReadDatas(int handle, const int point_id, const int type_id, const time_t time_begin, const time_t time_end, time_t *data_times, Serializer *datas, int *num)
{
	if (num == NULL || datas == NULL || data_times == NULL || time_begin > time_end)
		return MYDATA_INVALIDE_PARA;
	int ret;
	for (int i=0; i<*num; i++)
	{
		ret = MakeSerializer(handle, type_id, false, &datas[i]);
		if (ret)
			return ret;
	}
	ClientInfo *cinfo = NULL;
	ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;
	ClientScopeLock(cinfo);
	cln_proto_handler client_handler(cinfo->GetRoot());
	Datas::ReadDatasQuery query;
	query.set_id(point_id);
	query.set_from_time(time_end);
	query.set_time_begin(time_begin);
	query.set_time_end(time_end);
	int i = 0;
	while (ret == 0)
	{
		ret = client_handler.send_all(*cinfo, query, MsgReadDatasQuery);
		if (ret)
			break;

		Common::Head head;
		Datas::ReadDatasReply &reply = cinfo->read_reply_;
		ret = client_handler.recv_all(*cinfo, MsgReadDatasReply, head, reply);
		if (ret)
			break;

		auto array = reply.data_infos();
		for (auto iter = array.begin(); iter != array.end() && i < *num; ++iter, ++i)
		{
			data_times[i] = iter->time_stamp();
			memcpy(datas[i].buf, iter->data().c_str(), iter->data().length());
		}
		
		if (data_times[i-1] - 1 < time_begin)
		{
			ret = MYDATA_OK;
			break;
		}
		query.set_from_time(data_times[i-1] - 1);
		ret = reply.err();
		reply.Clear();
	}
	*num = i;
	if (ret == MYDATA_DATA_QUERY_END)
		ret = MYDATA_OK;
	cinfo->read_reply_.Clear();
	return 0;
}

MYDATA_API int WriteOldDatas(int handle, const int point_id,  const time_t *times, const Serializer *datas, int *num)
{
	if (times == NULL || datas == NULL || num == NULL || *num < 1)
		return MYDATA_INVALIDE_PARA;

	ClientInfo *cinfo = NULL;
	int ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;

	ClientScopeLock(cinfo);
	cln_proto_handler client_handler(cinfo->GetRoot());
	Datas::WriteOldDatasQuery &query = cinfo->write_old_query_;
	query.set_id(point_id);
	auto data_infos = query.mutable_data_infos();
	data_infos->Reserve(*num);
	int length = 0;
	const void *data = 0;
	for (int i=0; i<*num; i++)
	{
		data = GetSerializeBuf(datas+i, &length);
		if (data == NULL)
			continue;
		auto data_info = data_infos->Add();
		data_info->set_time_stamp(times[i]);
		data_info->set_data(data, length);
	}
	ret = client_handler.send_all(*cinfo, query, MsgWriteOldDatasQuery);
	query.Clear();
	if (ret)
		return ret;

	Common::Head head;
	Datas::WriteOldDatasReply reply;
	ret = client_handler.recv_all(*cinfo, MsgWriteOldDatasReply, head, reply);
	if (ret)
		return ret;
	ret = reply.err();
	if (ret == 0 && *num != reply.num())
		ret = MYDATA_DATA_PAR_SUC;
	return ret;
}

MYDATA_API int RemoveDatas(int handle, const int point_id,  const time_t *times, int num)
{
	if (times == NULL)
		return MYDATA_INVALIDE_PARA;

	ClientInfo *cinfo = NULL;
	int ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;

	ClientScopeLock(cinfo);
	cln_proto_handler client_handler(cinfo->GetRoot());
	Datas::RemoveDatasQuery query;
	query.set_id(point_id);
	auto timestamps = query.mutable_time_stamps();
	timestamps->Reserve(num);
	for (int i=0; i<num; i++)
	{
		timestamps->Add(times[i]);
	}
	ret = client_handler.send_all(*cinfo, query, MsgRemoveDatasQuery);
	if (ret)
		return ret;

	Common::Head head;
	Datas::RemoveDatasReply reply;
	ret = client_handler.recv_all(*cinfo, MsgRemoveDatasReply, head, reply);
	if (ret)
		return ret;
	return reply.err();
}

MYDATA_API int GetDataFileNums(int handle, int *num)
{
	if (num == NULL)
		return MYDATA_INVALIDE_PARA;
	ClientInfo *cinfo = NULL;
	int ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;
	ClientScopeLock(cinfo);
	cln_proto_handler client_handler(cinfo->GetRoot());
	Datas::GetDataFileNumsQuery query;
	query.set_unused(0);
	ret = client_handler.send_all(*cinfo, query, MsgGetDataFileNumsQuery);
	if (ret)
		return ret;

	Common::Head head;
	Datas::GetDataFileNumsReply reply;
	ret = client_handler.recv_all(*cinfo, MsgGetDataFileNumsReply, head, reply);
	if (ret)
		return ret;
	ret = reply.err();
	if (ret)
		return ret;
	*num = reply.num();
	return ret;
}

MYDATA_API int GetDataFileInfos(int handle, DataFileInfo *data_file_infos, int *num)
{
	if (data_file_infos == NULL || num == NULL || *num <= 0)
		return MYDATA_INVALIDE_PARA;
	ClientInfo *cinfo = NULL;
	int ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;

	ClientScopeLock(cinfo);
	cln_proto_handler client_handler(cinfo->GetRoot());
	Datas::DataFileInfosQuery query;
	query.set_num(*num);
	ret = client_handler.send_all(*cinfo, query, MsgDataFileInfosQuery);
	if (ret)
		return ret;

	Common::Head head;
	Datas::DataFileInfosReply reply;
	ret = client_handler.recv_all(*cinfo, MsgDataFileInfosReply, head, reply);
	if (ret)
		return ret;
	ret = reply.err();
	if (ret)
		return ret;

	auto array = reply.data_file_infos();
	int i = 0;
	for (auto iter = array.begin(); iter != array.end() && i<*num; ++iter, ++i)
	{
		SafeCopy(iter->name(), data_file_infos[i].filename, NAME_SIZE);
		data_file_infos[i].file_size = iter->size();
		data_file_infos[i].using_rate = iter->using_rate();
		data_file_infos[i].start_time = iter->start_time();
		data_file_infos[i].end_time = iter->end_time();
	}
	*num = i;
	return ret;
}

MYDATA_API int MyDataAddFile(int handle, const char *filename)
{
	if (filename == NULL || strlen(filename) >= NAME_SIZE)
		return MYDATA_INVALIDE_PARA;

	ClientInfo *cinfo = NULL;
	int ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;
	ClientScopeLock(cinfo);
	cln_proto_handler client_handler(cinfo->GetRoot());
	Datas::AddFileQuery query;
	query.set_filename(filename);
	ret = client_handler.send_all(*cinfo, query, MsgAddFileQuery);
	if (ret)
		return ret;

	Common::Head head;
	Datas::AddFileReply reply;
	ret = client_handler.recv_all(*cinfo, MsgAddFileReply, head, reply);
	if (ret)
		return ret;
	ret = reply.err();
	return ret;
}

MYDATA_API int MyDataCreateFile(int handle, const char *filename, time_t start_time, time_t end_time, long long filesize)
{
	if (filename == NULL || strlen(filename) >= NAME_SIZE || filesize < 1048576*100 || filesize > 1048576LL*1024*8)
		return MYDATA_INVALIDE_PARA;

	ClientInfo *cinfo = NULL;
	int ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;
	ClientScopeLock(cinfo);
	cln_proto_handler client_handler(cinfo->GetRoot());
	Datas::CreateFileQuery query;
	query.set_filename(filename);
	query.set_size(filesize);
	query.set_start_time(start_time);
	query.set_end_time(end_time);
	ret = client_handler.send_all(*cinfo, query, MsgCreateFileQuery);
	if (ret)
		return ret;

	Common::Head head;
	Datas::CreateFileReply reply;
	ret = client_handler.recv_all(*cinfo, MsgCreateFileReply, head, reply);
	if (ret)
		return ret;
	ret = reply.err();
	return ret;
}

MYDATA_API int MyDataDeleteFile(int handle, const char *filename, time_t start_time)
{
	if (filename == NULL || strlen(filename) >= NAME_SIZE)
		return MYDATA_INVALIDE_PARA;

	ClientInfo *cinfo = NULL;
	int ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;
	ClientScopeLock(cinfo);
	cln_proto_handler client_handler(cinfo->GetRoot());
	Datas::DeleteFileQuery query;
	query.set_file_name(filename);
	query.set_start_time(start_time);
	ret = client_handler.send_all(*cinfo, query, MsgDeleteFileQuery);
	if (ret)
		return ret;

	Common::Head head;
	Datas::DeleteFileReply reply;
	ret = client_handler.recv_all(*cinfo, MsgDeleteFileReply, head, reply);
	if (ret)
		return ret;
	ret = reply.err();
	return ret;
}

MYDATA_API int MyDataRebuildIndex(int handle, const char *filename, time_t start_time)
{
	if (filename == NULL || strlen(filename) >= NAME_SIZE)
		return MYDATA_INVALIDE_PARA;

	ClientInfo *cinfo = NULL;
	int ret = ClientInfo::GetClientInfo(handle, cinfo);
	if (ret != MYDATA_OK)
		return ret;
	ClientScopeLock(cinfo);
	cln_proto_handler client_handler(cinfo->GetRoot());
	Datas::RebuildIndexQuery query;
	query.set_filename(filename);
	query.set_start_time(start_time);
	ret = client_handler.send_all(*cinfo, query, MsgRebuildIndexQuery);
	if (ret)
		return ret;

	Common::Head head;
	Datas::RebuildIndexReply reply;
	ret = client_handler.recv_all(*cinfo, MsgRebuildIndexReply, head, reply);
	if (ret)
		return ret;
	ret = reply.err();
	return ret;
}
