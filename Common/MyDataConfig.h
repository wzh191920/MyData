#pragma once
class MyDataConfig
{
public:
	void Init();
	~MyDataConfig();
	long long _auto_create_size;
	int _port;
	int _thread_num;
	char* _log_path;
	char* _data_path;
};

