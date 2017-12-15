#include "stdafx.h"
#include "ClientProtoHandler.h"
#include "ace/SOCK_Connector.h"
#include "ace/Time_Value.h"

cln_proto_handler::cln_proto_handler(MEM_ROOT* root):proto_handler(root)
{
}

int cln_proto_handler::recv_head(ClientInfo &info, Common::Head &head)
{
	int ret = proto_handler::recv_head(info.GetSocket(), head);
	if (ret)
	{
		reconnect(info);
	}
	return ret;
}
int cln_proto_handler::recv_head(ClientInfo &info, int want_type, Common::Head &head)
{
	int ret = proto_handler::recv_head(info.GetSocket(), want_type, head);
	if (ret)
	{
		reconnect(info);
	}
	return ret;
}

void cln_proto_handler::reconnect(ClientInfo &info)
{
	if (info.IsConnect())
	{
		info.CloseSock();
		info.IsConnect(false);
	}
	ACE_SOCK_Stream &sock = info.GetSocket();
	ACE_SOCK_Connector con;
	ACE_INET_Addr addr(info.GetPort(), info.GetIp());
	ACE_Time_Value timeout(5, 0);
	int ret = con.connect(sock, addr, &timeout, ACE_Addr::sap_any, 1);
	if (ret == 0)
		info.IsConnect(true);
}
