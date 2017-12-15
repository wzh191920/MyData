#pragma once
#include "ace/Asynch_IO.h"
#include "ace/OS.h"
#include "ace/Asynch_Acceptor.h"
#include "ace/Proactor.h"
#include "proto_handler.h"
#include "datas.pb.h"

class MyDataServiceHandler : public ACE_Service_Handler
{
public:
	~MyDataServiceHandler()
	{
		free_root(&root_, MY_MARK_BLOCKS_FREE);
		if (handle() != INVALID_HANDLE_VALUE)
			ACE_OS::closesocket(handle());
	}
	virtual void open(ACE_HANDLE h, ACE_Message_Block&);
	virtual void handle_read_stream(const ACE_Asynch_Read_Stream::Result &result);
	virtual void handle_write_stream(const ACE_Asynch_Write_Stream::Result &result);
private:
	static void* handle_input_task(void* data);
	int handle_head();
	int handle_body();
	int read_head();
	template<typename T>
	int send_all(T &reply, int btype)
	{
		int allsize = 0;
		char *allbuf = NULL;
		int ret = proto_handler(&root_).send_all_proto(reply, btype, allbuf, allsize);
		if (ret)
			return ret;
		mb_.init(allbuf, allsize);
		mb_.wr_ptr(allsize);
		if (writer_.write(mb_, allsize) != 0)
		{
			return MYDATA_SEND_FAIL;
		}
		return MYDATA_OK;
	}
	int handle_connect_msg();
	int handle_add_type_msg();
	int handle_delete_type_msg();
	int handle_get_typesnum_msg();
	int handle_get_types_msg();
	int handle_get_type_byid_msg();
	int handle_add_table_msg();
	int handle_delete_table_msg();
	int handle_get_tablesnum_msg();
	int handle_get_tables_msg();
	int handle_get_table_byid_msg();
	int handle_add_points_msg();
	int handle_delete_point_msg();
	int handle_search_pointsnum_msg();
	int handle_search_pointsnum_byid_msg();
	int handle_search_points_msg();
	int handle_search_points_byid_msg();
	int handle_write_new_datas_msg();
	int handle_read_new_datas_msg();
	int handle_read_datas_num_msg();
	int handle_read_datas_msg();
	int handle_get_datafile_nums_msg();
	int handle_get_datafile_infos_msg();
	int handle_write_old_datas_msg();
	int handle_remove_datas_msg();
	int handle_add_file_msg();
	int handle_create_file_msg();
	int handle_delete_file_msg();
	int handle_rebuild_index_msg();
private:
	ACE_Asynch_Read_Stream reader_;
	ACE_Asynch_Write_Stream writer_;
	MEM_ROOT root_;
	ACE_Message_Block mb_;
	Common::Head head_;
	Datas::WriteNewDatasQuery write_new_query_;
	Datas::WriteOldDatasQuery write_old_query_;
	Datas::ReadNewDatasQuery read_new_query_;
	Datas::ReadNewDatasReply read_new_reply_;
	Datas::ReadDatasReply read_reply_;
	bool ishead_;
};

