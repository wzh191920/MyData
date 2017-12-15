#include "stdafx.h"
#include "proto_handler.h"
#include "ace/Time_Value.h"

int proto_handler::_head_size = 0;
int proto_handler::_head_flag_size = 0;
ACE_Time_Value proto_handler::_timeout(180);

proto_handler::proto_handler(MEM_ROOT *root):root_(root)
{
}
void proto_handler::init()
{
	Common::Head head;
	Common::HeadFlag head_flag;
	head.set_bodycrc(0);
	head.set_bodysize(0);
	head.set_bodytype(0);

	head_flag.set_headcrc(0);
	head_flag.set_magic(0);

	_head_size = head.ByteSize();
	_head_flag_size = head_flag.ByteSize();
}
int proto_handler::get_heads_size()
{
	return _head_flag_size+_head_size;
}
int proto_handler::clear_sockbuf(ACE_SOCK_Stream &sock, int &size)
{
	char *buf = new char[size];
	unique_ptr<char[]> uptr(buf);

	int ret = sock.recv_n(buf, size, &_timeout);
	if (ret != size)
	{
		ret = errno;
		if (ret == ETIME)
			return MYDATA_NET_TIMEOUT;
		else
			return MYDATA_RECV_FAIL;
	}
	return MYDATA_OK;
}
int proto_handler::recv_head(ACE_SOCK_Stream &sock, Common::Head &head)
{
	int headall_size = _head_size+_head_flag_size;
	char *headbuf = (char*)alloc_root(root_, headall_size);

	int ret = sock.recv_n(headbuf, headall_size, &_timeout);
	if (ret != headall_size)
	{
		ret = errno;
		if (ret == ETIME)
			return MYDATA_NET_TIMEOUT;
		else
			return MYDATA_RECV_FAIL;
	}
	
	return recv_head_proto(head, headbuf);
}
int proto_handler::recv_head(ACE_SOCK_Stream &sock, int want_type, Common::Head &head)
{
	int ret = recv_head(sock, head);
	if (ret == 0 && head.bodytype() != want_type)
	{
		return MYDATA_ERROR_TYPE;
	}
	else
	{
		return ret;
	}
}

int proto_handler::recv_head_proto(Common::Head &head, char* headbuf)
{
	Common::HeadFlag head_flag;
	head_flag.ParseFromArray(headbuf, _head_flag_size);
	unsigned int hcrc = ACE::crc32(headbuf+_head_flag_size, _head_size);
	if (head_flag.magic() != HEAD_MAGIC || head_flag.headcrc() != hcrc)
	{
		return MYDATA_CRC_FAIL;
	}

	if (head.ParseFromArray(headbuf+_head_flag_size, _head_size))
		return MYDATA_OK;
	return MYDATA_MSG_SER_FAIL;
}