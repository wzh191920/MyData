#include "StdAfx.h"
#include "Service.h"
#include "ace/Message_Block.h"
#include "threadpoolw.h"
#include "SqlHandler.h"
#include "LogFilew.h"
#include "MyDataCommon.h"
#include "PlugIn.h"
#include "net.pb.h"
#include "points.pb.h"
#include "datas.pb.h"
#include "DataMgr.h"
extern threadpool _tp;
extern SqlHandler _sql_handler;
extern DataMgr _dm;
extern PlugIn _plugin;
class PointExistFun : public MyDataFun
{
public:
	virtual int MyDataCallBack(void* para)
	{
		SqlHandler* sql_handler = (SqlHandler*)data_;
		int *point_id = (int*)para;
		return sql_handler->IsPointExist(*point_id);
	}
};

void MyDataServiceHandler::open(ACE_HANDLE h, ACE_Message_Block&)
{
	handle(h);
	if (reader_.open(*this) != 0 || writer_.open(*this) != 0)
	{
		delete this;
		return;
	}
	init_alloc_root(&root_, 4096, 4096);
	if (read_head())
	{
		LOG_ERROR_WRITE("读取网络数据失败");
		delete this;
		return;
	}
	return;
}
int MyDataServiceHandler::read_head()
{
	int heads_size = proto_handler::get_heads_size();
	char* headbuf = (char*)alloc_root(&root_, heads_size);
	mb_.init(headbuf, heads_size);
	ishead_ = true;
	if (reader_.read(mb_, heads_size) != 0)
		return -1;
	return MYDATA_OK;
}
void MyDataServiceHandler::handle_read_stream(const ACE_Asynch_Read_Stream::Result &result)
{
	int trans = result.bytes_transferred();   //实际读取的字节数
	int want = result.bytes_to_read();        //想要读取的字节数
	if (!result.success() || trans==0)
	{
		delete this;
		return;
	}
	if (trans < want)
	{
		if (reader_.read(mb_, want-trans))
		{
			LOG_ERROR_WRITE("读取网络数据失败");
			delete this;
		}
		return;
	}
	thread_task task;
	task.callback_fun = handle_input_task;
	task.data = this;
	_tp.puttask(task);
}

void MyDataServiceHandler::handle_write_stream(const ACE_Asynch_Write_Stream::Result &result)
{
	int length = mb_.length();
	if (length > 0)
	{
		if (writer_.write(mb_, length))
		{
			LOG_ERROR_WRITE("发送网络数据失败");
			delete this;
		}
		return;
	}
	free_root(&root_, MY_MARK_BLOCKS_FREE);
	if (read_head())
	{
		LOG_ERROR_WRITE("读取网络数据失败");
		delete this;
	}
}

void* MyDataServiceHandler::handle_input_task(void* data)
{
	MyDataServiceHandler *h = (MyDataServiceHandler*)data;

	if (h->ishead_)
	{
		h->handle_head();
	}
	else
	{
		h->handle_body();
	}
	
	return 0;
}

int MyDataServiceHandler::handle_head()
{
	char *headbuf = mb_.rd_ptr();
	int ret = proto_handler::recv_head_proto(head_, headbuf);
	if (ret)
	{
		LOG_ERROR_WRITE("接收头失败, 错误码:%d", ret);
		delete this;
		return ret;
	}
	ishead_ = false;
	int bodysize = head_.bodysize();
	mb_.init((const char*)alloc_root(&root_, bodysize), bodysize);
	if (reader_.read(mb_, bodysize) != 0)
	{
		LOG_ERROR_WRITE("接收头失败, 错误码:%d", ret);
		delete this;
		return -1;
	}
	
	return ret;
}

int MyDataServiceHandler::handle_body()
{
	int ret = MYDATA_OK;
	switch (head_.bodytype())
	{
	case MsgWriteNewDatasQuery:
		ret = handle_write_new_datas_msg();
		break;
	case MsgReadNewDatasQuery:
		ret = handle_read_new_datas_msg();
		break;
	case MsgReadDatasNumQuery:
		ret = handle_read_datas_num_msg();
		break;
	case MsgReadDatasQuery:
		ret = handle_read_datas_msg();
		break;
	case MsgWriteOldDatasQuery:
		ret = handle_write_old_datas_msg();
		break;
	case MsgConnectQuery:
		ret = handle_connect_msg();
		break;
	case MsgAddTypeQuery:
		ret = handle_add_type_msg();
		break;
	case MsgDeleteTypeQuery:
		ret = handle_delete_type_msg();
		break;
	case MsgGetTypesNumQuery:
		ret = handle_get_typesnum_msg();
		break;
	case MsgGetTypesQuery:
		ret = handle_get_types_msg();
		break;
	case MsgAddTableQuery:
		ret = handle_add_table_msg();
		break;
	case MsgDeleteTableQuery:
		ret = handle_delete_table_msg();
		break;
	case MsgGetTablesNumQuery:
		ret = handle_get_tablesnum_msg();
		break;
	case MsgGetTablesQuery:
		ret = handle_get_tables_msg();
		break;
	case MsgGetTableByIDQuery:
		ret = handle_get_table_byid_msg();
		break;
	case MsgAddPointQuery:
		ret = handle_add_points_msg();
		break;
	case MsgDeletePointQuery:
		ret = handle_delete_point_msg();
		break;
	case MsgSearchPointsNumQuery:
		ret = handle_search_pointsnum_msg();
		break;
	case MsgSearchPointsQuery:
		ret = handle_search_points_msg();
		break;
	case MsgSearchPointsNumBYIDQuery:
		ret = handle_search_pointsnum_byid_msg();
		break;
	case MsgSearchPointsByIDQuery:
		ret = handle_search_points_byid_msg();
		break;
	case MsgGetTypeByIDQuery:
		ret = handle_get_type_byid_msg();
		break;
	case MsgGetDataFileNumsQuery:
		ret = handle_get_datafile_nums_msg();
		break;
	case MsgDataFileInfosQuery:
		ret = handle_get_datafile_infos_msg();
		break;
	case MsgRemoveDatasQuery:
		ret = handle_remove_datas_msg();
		break;
	case MsgAddFileQuery:
		ret = handle_add_file_msg();
		break;
	case MsgCreateFileQuery:
		ret = handle_create_file_msg();
		break;
	case MsgDeleteFileQuery:
		ret = handle_delete_file_msg();
		break;
	case MsgRebuildIndexQuery:
		ret = handle_rebuild_index_msg();
		break;
	default:
		LOG_ERROR_WRITE("没有消息类型%d", head_.bodytype());
		ret = MYDATA_ERROR_TYPE;
	}
	if (ret < 0)
	{
		//if (ret != MYDATA_CRC_FAIL)
		{
			LOG_ERROR_WRITE("处理体消息失败, 错误码:%d", ret);
			delete this;
		}
	}
	return ret;
}

int MyDataServiceHandler::handle_connect_msg()
{
	char *bodybuf = mb_.rd_ptr();
	MydataNet::ConnectQuery query;
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;
	MydataNet::ConnectReply reply;
	reply.set_err(0);
	ret = send_all(reply, MsgConnectReply);
	return ret;
}

int MyDataServiceHandler::handle_add_type_msg()
{
	char *bodybuf = mb_.rd_ptr();
	Points::AddTypeQuery query;
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;

	Points::AddTypeReply reply;
	_sql_handler.AddType(query, reply);
	ret = send_all(reply, MsgAddTypeReply);
	return ret;
}

int MyDataServiceHandler::handle_delete_type_msg()
{
	char *bodybuf = mb_.rd_ptr();
	Points::DeleteTypeQuery query;
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;
	
	Points::DeleteTypeReply reply;
	_sql_handler.DeleteType(query, reply);
	ret = send_all(reply, MsgDeleteTypeReply);
	return ret;
}

int MyDataServiceHandler::handle_get_typesnum_msg()
{
	char *bodybuf = mb_.rd_ptr();
	Points::GetTypesNumQuery query;
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;

	Points::GetTypesNumReply reply;
	_sql_handler.GetTypesNum(reply);
	ret = send_all(reply, MsgGetTypesNumReply);
	return ret;
}

int MyDataServiceHandler::handle_get_types_msg()
{
	char *bodybuf = mb_.rd_ptr();
	Points::GetTypesQuery query;
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;

	Points::GetTypesReply reply;
	_sql_handler.GetTypes(query, reply);
	ret = send_all(reply, MsgGetTypesReply);
	return ret;
}

int MyDataServiceHandler::handle_get_type_byid_msg()
{
	char *bodybuf = mb_.rd_ptr();
	Points::GetTypeByIDQuery query;
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;

	Points::GetTypeByIDReply reply;
	_sql_handler.GetTypeByID(query, reply);
	ret = send_all(reply, MsgGetTypeByIDReply);
	return ret;
}

int MyDataServiceHandler::handle_add_table_msg()
{
	char *bodybuf = mb_.rd_ptr();
	Points::AddTableQuery query;
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;

	Points::AddTableReply reply;
	_sql_handler.AddTable(query, reply);
	ret = send_all(reply, MsgAddTableReply);
	return ret;
}
int MyDataServiceHandler::handle_delete_table_msg()
{
	char *bodybuf = mb_.rd_ptr();
	Points::DeleteTableQuery query;
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;

	Points::DeleteTableReply reply;
	_sql_handler.DeleteTable(query, reply);
	ret = send_all(reply, MsgDeleteTableReply);
	return ret;
}
int MyDataServiceHandler::handle_get_tablesnum_msg()
{
	char *bodybuf = mb_.rd_ptr();
	Points::GetTablesNumQuery query;
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;

	Points::GetTablesNumReply reply;
	_sql_handler.GetTablesNum(reply);
	ret = send_all(reply, MsgGetTablesNumReply);
	return ret;
}
int MyDataServiceHandler::handle_get_tables_msg()
{
	char *bodybuf = mb_.rd_ptr();
	Points::GetTablesQuery query;
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;

	Points::GetTablesReply reply;
	_sql_handler.GetTables(query, reply);
	ret = send_all(reply, MsgGetTablesReply);
	return ret;
}

int MyDataServiceHandler::handle_get_table_byid_msg()
{
	char *bodybuf = mb_.rd_ptr();
	Points::GetTableByIDQuery query;
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;
	
	Points::GetTableByIDReply reply;
	_sql_handler.GetTableByID(query, reply);
	ret = send_all(reply, MsgGetTableByIDReply);
	return ret;
}

int MyDataServiceHandler::handle_add_points_msg()
{
	char *bodybuf = mb_.rd_ptr();
	Points::AddPointsQuery query;
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;

	Points::AddPointsReply reply;
	_sql_handler.AddPoints(query, reply);
	ret = send_all(reply, MsgAddPointReply);
	return ret;
}
int MyDataServiceHandler::handle_delete_point_msg()
{
	char *bodybuf = mb_.rd_ptr();
	Points::DeletePointQuery query;
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;

	Points::DeletePointReply reply;
	_sql_handler.DeletePoint(query, reply);
	//_dm.RemovePoint(query.id());
	//_sql_handler.FinishDeletePoint(query, reply);
	ret = send_all(reply, MsgDeletePointReply);
	return ret;
}
int MyDataServiceHandler::handle_search_pointsnum_msg()
{
	char *bodybuf = mb_.rd_ptr();
	Points::SearchPointsNumQuery query;
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;

	Points::SearchPointsNumReply reply;
	_sql_handler.SearchPointsNum(query, reply);
	ret = send_all(reply, MsgSearchPointsNumReply);
	return ret;
}
int MyDataServiceHandler::handle_search_points_msg()
{
	char *bodybuf = mb_.rd_ptr();
	Points::SearchPointsQuery query;
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;

	Points::SearchPointsReply reply;
	_sql_handler.SearchPoints(query, reply);
	ret = send_all(reply, MsgSearchPointsReply);
	return ret;
}

int MyDataServiceHandler::handle_search_pointsnum_byid_msg()
{
	char *bodybuf = mb_.rd_ptr();
	Points::SearchPointsNumByIDQuery query;
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;

	Points::SearchPointsNumReply reply;
	_sql_handler.SearchPointsNumByID(query, reply);
	ret = send_all(reply, MsgSearchPointsNumReply);
	return ret;
}

int MyDataServiceHandler::handle_search_points_byid_msg()
{
	char *bodybuf = mb_.rd_ptr();
	Points::SearchPointsByIDQuery query;
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;

	Points::SearchPointsReply reply;
	_sql_handler.SearchPointsByID(query, reply);
	ret = send_all(reply, MsgSearchPointsReply);
	return ret;
}

int MyDataServiceHandler::handle_write_new_datas_msg()
{
	char *bodybuf = mb_.rd_ptr();
	Datas::WriteNewDatasQuery &query = write_new_query_;
	query.Clear();
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;

	Datas::WriteNewDatasReply reply;
	reply.set_err(MYDATA_OK);
	auto array = query.data_infos();
	int num = 0;
	PointExistFun function;
	function.data_ = &_sql_handler;
	if (_plugin.WriteNewDataFuns_.empty())
	{//没有插件
		for (auto iter = array.begin(); iter != array.end(); ++iter)
		{
			ret = _dm.WriteNewData(iter->id(), iter->time_stamp(), iter->data().c_str(), iter->data().length(), &function);
			if (ret)
				reply.set_err(ret);
			else
				num++;
		}
	}
	else
	{//有插件
		bool write_in = true;
		for (auto iter = array.begin(); iter != array.end(); ++iter)
		{
			for (auto fun_iter = _plugin.WriteNewDataFuns_.begin(); fun_iter != _plugin.WriteNewDataFuns_.end(); ++fun_iter)
			{
				if (!(*fun_iter)(iter->table_id(), iter->id(), iter->time_stamp(), iter->data().c_str()))
				{
					write_in = false;
					reply.set_err(MYDATA_PLUGIN_PREVENT);
					break;
				}
			}
			if (write_in)
			{
				ret = _dm.WriteNewData(iter->id(), iter->time_stamp(), iter->data().c_str(), iter->data().length(), &function);
				if (ret)
					reply.set_err(ret);
				else
					num++;
			}
			else
				write_in = true;
		}
	}
	reply.set_num(num);
	ret = send_all(reply, MsgWriteNewDatasReply);
	return ret;
}

int MyDataServiceHandler::handle_read_new_datas_msg()
{
	char *bodybuf = mb_.rd_ptr();
	Datas::ReadNewDatasQuery &query = read_new_query_;
	query.Clear();
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;

	Datas::ReadNewDatasReply &reply = read_new_reply_;
	reply.Clear();
	auto ids_array = query.ids();
	auto datas_infos_array = reply.mutable_data_infos();
	reply.set_err(MYDATA_OK);
	datas_infos_array->Reserve(ids_array.size());

	int num = 0;
	time_t t = -1;
	char data_buf[MAX_DATA_SIZE] = {0};
	int length = 1;
	for (auto iter = ids_array.begin(); iter != ids_array.end(); ++iter)
	{
		ret = _dm.ReadNewData(*iter, t, data_buf, length);
		auto data_info = datas_infos_array->Add();
		data_info->set_data(data_buf, length);
		data_info->set_time_stamp(t);
		if (ret == MYDATA_OK)
		{
			t = -1;
			data_buf[0] = '\0';
			length = 1;
			num++;
		}
		else
		{
			reply.set_err(MYDATA_DATA_PAR_SUC);
		}
	}
	reply.set_num(num);
	ret = send_all(reply, MsgReadNewDatasReply);
	return ret;
}

int MyDataServiceHandler::handle_read_datas_num_msg()
{
	char *bodybuf = mb_.rd_ptr();
	Datas::ReadDatasNumQuery query;
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;

	Datas::ReadDatasNumReply reply;
	int num = 0;
	ret = _dm.ReadDatasNum(query.id(), query.time_begin(), query.time_end(), num);
	reply.set_err(ret);
	reply.set_num(num);
	ret = send_all(reply, MsgReadDatasNumReply);
	return ret;
}

int MyDataServiceHandler::handle_read_datas_msg()
{
	char *bodybuf = mb_.rd_ptr();
	Datas::ReadDatasQuery query;
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;

	Datas::ReadDatasReply &reply = read_reply_;
	reply.Clear();
	ret = _dm.ReadDatas(query.id(), query.time_begin(), query.from_time(), reply);
	reply.set_err(ret);
	ret = send_all(reply, MsgReadDatasReply);
	return ret;
}

int MyDataServiceHandler::handle_get_datafile_nums_msg()
{
	char *bodybuf = mb_.rd_ptr();
	Datas::GetDataFileNumsQuery query;
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;

	Datas::GetDataFileNumsReply reply;
	reply.set_num(_dm.GetFilesNum());
	reply.set_err(MYDATA_OK);
	ret = send_all(reply, MsgGetDataFileNumsReply);
	return ret;
}
int MyDataServiceHandler::handle_get_datafile_infos_msg()
{
	char *bodybuf = mb_.rd_ptr();
	Datas::DataFileInfosQuery query;
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;

	Datas::DataFileInfosReply reply;
	_dm.GetFileInfos(reply);
	reply.set_err(MYDATA_OK);
	ret = send_all(reply, MsgDataFileInfosReply);
	return ret;
}

int MyDataServiceHandler::handle_write_old_datas_msg()
{
	char *bodybuf = mb_.rd_ptr();
	Datas::WriteOldDatasQuery &query = write_old_query_;
	query.Clear();
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;
	int num = 0;
	PointExistFun function;
	function.data_ = &_sql_handler;
	ret = _dm.WriteOldDatas(query.id(), query, num, &function);
	Datas::WriteOldDatasReply reply;
	reply.set_err(ret);
	reply.set_num(num);
	ret = send_all(reply, MsgWriteOldDatasReply);
	return ret;
}

int MyDataServiceHandler::handle_remove_datas_msg()
{
	char *bodybuf = mb_.rd_ptr();
	Datas::RemoveDatasQuery query;
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;
	ret = _dm.RemoveDatas(query);
	Datas::RemoveDatasReply reply;
	reply.set_err(ret);
	ret = send_all(reply, MsgRemoveDatasReply);
	return ret;
}

int MyDataServiceHandler::handle_add_file_msg()
{
	char *bodybuf = mb_.rd_ptr();
	Datas::AddFileQuery query;
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;
	ret = _dm.AddFile(query.filename().c_str());
	Datas::AddFileReply reply;
	reply.set_err(ret);
	ret = send_all(reply, MsgAddFileReply);
	return ret;
}
int MyDataServiceHandler::handle_create_file_msg()
{
	char *bodybuf = mb_.rd_ptr();
	Datas::CreateFileQuery query;
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;
	ret = _dm.CreateFile(query.filename().c_str(), query.size(), query.start_time(), query.end_time());
	Datas::CreateFileReply reply;
	reply.set_err(ret);
	ret = send_all(reply, MsgCreateFileReply);
	return ret;
}
int MyDataServiceHandler::handle_delete_file_msg()
{
	char *bodybuf = mb_.rd_ptr();
	Datas::DeleteFileQuery query;
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;
	ret = _dm.DeleteFile(query.file_name().c_str(), query.start_time());
	Datas::DeleteFileReply reply;
	reply.set_err(ret);
	ret = send_all(reply, MsgDeleteFileReply);
	return ret;
}

int MyDataServiceHandler::handle_rebuild_index_msg()
{
	char *bodybuf = mb_.rd_ptr();
	Datas::RebuildIndexQuery query;
	int ret = proto_handler::recv_body_proto(head_, query, bodybuf);
	if (ret)
		return ret;
	ret = _dm.ReBuildIndex(query.filename().c_str(), query.start_time());
	Datas::RebuildIndexReply reply;
	reply.set_err(ret);
	ret = send_all(reply, MsgRebuildIndexReply);
	return ret;
}