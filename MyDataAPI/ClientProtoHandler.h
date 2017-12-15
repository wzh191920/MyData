
#ifndef CLIENT_PROTO_HANDLER_H_
#define CLIENT_PROTO_HANDLER_H_
#include "proto_handler.h"
#include "ApiCommon.h"
//处理包头和包体的类，是proto_handler的装饰类，比proto_handler类增加了socket重连
class cln_proto_handler : public proto_handler
{
public:
	cln_proto_handler(MEM_ROOT* root);
	int recv_head(ClientInfo &info, Common::Head &head);
	int recv_head(ClientInfo &info, int want_type, Common::Head &head);
	template<typename T>
	int recv_body(ClientInfo &info, Common::Head &head, T &pb)
	{
		int ret = proto_handler::recv_body(info.GetSocket(), head, pb);
		if (ret)
		{
			reconnect(info);
		}
		return ret;
	}
	template <typename T>
	int send_all(ClientInfo &info, T &pb, int btype)
	{
		int ret = proto_handler::send_all(info.GetSocket(), pb, btype);
		if (ret)
		{
			reconnect(info);
		}
		return ret;
	}
	template<typename T>
	int recv_all(ClientInfo &info, int want_type, Common::Head &head, T &pb)
	{
		int ret = recv_head(info, want_type, head);
		if (ret)
		{
			return ret;
		}
		ret = recv_body(info, head, pb);
		return ret;
	}
	void reconnect(ClientInfo &info);
};


#endif /* CLIENT_PROTO_HANDLER_H_ */
