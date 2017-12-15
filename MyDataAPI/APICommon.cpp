#include "stdafx.h"
#include "ApiCommon.h"

int ClientInfo::_max_handle = 0;
map<int, ClientInfo*> ClientInfo::_handles;
ACE_Thread_Mutex ClientInfo::_handles_mutex;
map<int, TypeInfo> _types;

ClientInfo::ClientInfo(const char* ip, int port)
{
	ACE_Guard<ACE_Thread_Mutex> g(_handles_mutex);
	handle_ = ++_max_handle;
	int len = strlen(ip);
	if (len > IP_LENGTH-1)
		len = IP_LENGTH-1;
	memset(ip_, 0 , sizeof(ip_));
	strncpy(ip_, ip, len);
	port_ = port;
	ref_ = 0;
	isconnect_ = false;
	init_alloc_root(&root_, 4096, 4096);
}
ClientInfo::~ClientInfo()
{
	sock_.close();
	free_root(&root_, MY_MARK_BLOCKS_FREE);
}
int ClientInfo::GetClientInfo(int handle, ClientInfo *&info)
{
	ACE_Guard<ACE_Thread_Mutex> g(_handles_mutex);
	map<int, ClientInfo*>::iterator iter = _handles.find(handle);
	if (iter != _handles.end())
	{
		info = iter->second;
		if (info->ref_ > 0)
			return MYDATA_HANDLE_CONFLICT;
		return MYDATA_OK;
	}
	return MYDATA_INVALIDE_HANDLE;
}

void ClientInfo::AddClientInfo()
{
	ACE_Guard<ACE_Thread_Mutex> g(_handles_mutex);
	_handles.insert(map<int, ClientInfo*>::value_type(handle_, this));
}

void ClientInfo::RemoveClientInfo()
{
	ACE_Guard<ACE_Thread_Mutex> g(_handles_mutex);
	_handles.erase(handle_);
}

MEM_ROOT* ClientInfo::GetRoot()
{
	return &root_;
}

void SafeCopy(const string &res, char* des, int len)
{
	int rlen = res.length();
	if (rlen >= len)
	{
		rlen = len - 1;
	}
	res.copy(des, rlen);
	des[rlen] = '\0';
}

char *StrReplace(const char* res, char* des, char r, char d)
{
	char *sd = des;
	while (*res != '\0')
	{
		if (*res == r)
			*des = d;
		else
			*des = *res;
		res++;
		des++;
	}
	*des = '\0';
	return sd;
}