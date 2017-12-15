#ifndef API_COMMON_H_
#define API_COMMON_H_

#include <map>
#include <string>
#include "ace/SOCK_Stream.h"
#include "ace/Thread_Mutex.h"
#include "ace/Guard_T.h"
#include "datas.pb.h"
#include "MyDataError.h"
#include "MyDataCommon.h"
#include "my_alloc.h"


using std::map;
using std::string;

//api的信息
class ClientInfo
{
public:
	ClientInfo(const char* ip, int port);
	~ClientInfo();
	ACE_SOCK_Stream& GetSocket() {return sock_;}
	int GetHandle() {return handle_;}
	bool IsConnect() {return isconnect_;}
	void IsConnect(bool con) {isconnect_ = con;}
	void CloseSock() {sock_.close();}
	int GetPort() {return port_;}
	char* GetIp() {return ip_;}
	void AddClientInfo();
	void RemoveClientInfo();
	MEM_ROOT* GetRoot();
	static int GetClientInfo(int handle, ClientInfo *&info);
	friend class ClientUseRef;
	Datas::WriteNewDatasQuery write_new_query_;
	Datas::WriteOldDatasQuery write_old_query_;
	Datas::ReadNewDatasQuery read_new_query_;
	Datas::ReadNewDatasReply read_new_reply_;
	Datas::ReadDatasReply read_reply_;
private:
	ACE_SOCK_Stream sock_;						//与服务端通信的socket
	char ip_[IP_LENGTH];						//服务端的ip
	int port_;									//服务端的端口
	int handle_;								//与socket对应的句柄，由客户端使用
	int ref_;									//用于检查句柄是否正在使用
	MEM_ROOT root_;
	bool isconnect_;
	static int _max_handle;						//用于生成句柄
	static map<int, ClientInfo*> _handles;		//socket句柄与socket的映射
	static ACE_Thread_Mutex _handles_mutex;		//访问_handles的锁
};
//客户端句柄使用的引用计数类
class ClientUseRef
{
public:
	ClientUseRef(ClientInfo* cinfo):cinfo_(cinfo)
	{
		++cinfo->ref_;
	}
	~ClientUseRef()
	{
		--cinfo_->ref_;
		free_root(cinfo_->GetRoot(), MY_MARK_BLOCKS_FREE);
	}
private:
	ClientInfo* cinfo_;
};
#define ClientScopeLock(pinfo) \
	ClientUseRef cref(pinfo)

void SafeCopy(const string &res, char* des, int len);
char *StrReplace(const char* res, char* des, char r, char d);
#endif /* API_COMMON_H_ */
