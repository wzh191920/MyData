
#ifndef PROTO_HANDLER_H_
#define PROTO_HANDLER_H_

#include <memory>
#include "ace/SOCK_Stream.h"
#include "ace/ACE.h"
#include "MessageCommon.h"
#include "common.pb.h"
#include "my_alloc.h"
#include "MyDataError.h"
using std::unique_ptr;

/*
包头组成
|         包头1        |                  包头2             |
|  magic  |   headcrc  |   bodytype  |  bodysize  | bodycrc |
  包头标识  包头2的crc     包体类型     包体大小    包体crc

包体只含有所有数据信息，不含有标识、类型、大小等额外信息

*/
//处理包头和包体的类
class proto_handler
{
public:
	proto_handler(MEM_ROOT *root);
    //接收socket缓冲区的数据
	int clear_sockbuf(ACE_SOCK_Stream &sock, int &size);
    //接收包头数据，保存在head中
	int recv_head(ACE_SOCK_Stream &sock, Common::Head &head);
	int recv_head(ACE_SOCK_Stream &sock, int want_type, Common::Head &head);
    //接收包体数据，保存中pb中
	template <typename T>
	int recv_body(ACE_SOCK_Stream &sock, Common::Head &head, T &pb)
	{
		int bsize = head.bodysize();
		char *bodybuf = (char*)alloc_root(root_, bsize);
		int ret = sock.recv_n(bodybuf, bsize, &_timeout);
		if (ret != bsize)
		{
			ret = errno;
			if (ret == ETIME)
				return MYDATA_NET_TIMEOUT;
			else
				return MYDATA_RECV_FAIL;
		}

		return recv_body_proto(head, pb, bodybuf);
	}
    //根据pb发送消息（包括包头和包体）
	template <typename T>
	int send_all(ACE_SOCK_Stream &sock, T &pb, int btype)
	{
		char *allbuf = NULL;
		int all_size = 0;
		int ret = send_all_proto(pb, btype, allbuf, all_size);
		if (ret)
			return ret;
		ret = sock.send_n(allbuf, all_size, &_timeout);
		if (ret != all_size)
		{
			ret = errno;
			if (ret == ETIME)
				return MYDATA_NET_TIMEOUT;
			else
				return MYDATA_SEND_FAIL;
		}
		return MYDATA_OK;
	}

	static ACE_Time_Value _timeout;                             //收发数据的超时时间
	static void init();
	static int get_heads_size();
	template <typename T>
	static int recv_body_proto(Common::Head &head, T &pb, char *bodybuf)
	{
		unsigned int bcrc_t = ACE::crc32(bodybuf, head.bodysize());
		if (head.bodycrc() != bcrc_t)
		{
			return MYDATA_CRC_FAIL;
		}
		if (pb.ParseFromArray(bodybuf, head.bodysize()))
			return MYDATA_OK;
		return MYDATA_MSG_SER_FAIL;
	}
	template <typename T>
	int send_all_proto(T &pb, int btype, char *&allbuf, int &all_size)
	{
		int headall_size = _head_size+_head_flag_size;
		all_size = headall_size+pb.ByteSize();
		allbuf = (char*)alloc_root(root_, all_size);
		char *bodybuf = allbuf + headall_size;
		if (!pb.SerializeToArray(bodybuf, pb.GetCachedSize()))
			return MYDATA_MSG_SER_FAIL;
		unsigned int crc = ACE::crc32(bodybuf, pb.GetCachedSize());

		Common::Head head;
		Common::HeadFlag head_flag;
		head.set_bodycrc(crc);
		head.set_bodysize(pb.GetCachedSize());
		head.set_bodytype(btype);
		if (!head.SerializeToArray(allbuf+_head_flag_size, _head_size))
			return MYDATA_MSG_SER_FAIL;

		crc = ACE::crc32(allbuf+_head_flag_size, _head_size);
		head_flag.set_headcrc(crc);
		head_flag.set_magic(HEAD_MAGIC);
		if (!head_flag.SerializeToArray(allbuf, _head_flag_size))
			return MYDATA_MSG_SER_FAIL;
		return MYDATA_OK;
	}
	static int recv_head_proto(Common::Head &head, char* headbuf);
private:
	static int _head_size;                                      //包头2大小
	static int _head_flag_size;                                 //包头1大小
	MEM_ROOT *root_;
};


#endif /* PROTO_HANDLER_H_ */
