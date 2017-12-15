#include "StdAfx.h"
#include "MyDataConfig.h"
#include "windows.h"
#include "LogFilew.h"

void MyDataConfig::Init()
{
	char path[MAX_PATH];
	if( !GetModuleFileName( NULL, path, MAX_PATH ) )
	{
		return;
	}
	char *end_pos = strrchr(path, '\\');
	*end_pos = '\0';
	char filename[MAX_PATH];
	sprintf(filename, "%s/config.ini", path);
	const char* app = "Common";
	_auto_create_size = GetPrivateProfileInt(app, "auto_create_size", 1024*1024*1024, filename);
	_port = GetPrivateProfileInt(app, "port", 8182, filename);
	_thread_num = GetPrivateProfileInt(app, "thread_num", 30, filename);
	_log_path = new char[MAX_PATH];
	GetPrivateProfileString(app, "log_path", "", _log_path, MAX_PATH, filename);
	_data_path = new char[MAX_PATH];
	GetPrivateProfileString(app, "data_path", "", _data_path, MAX_PATH, filename);
	
	if (strlen(_log_path) == 0)
	{
		sprintf(_log_path, "%s/log", path);
	}
	if (strlen(_data_path) == 0)
	{
		sprintf(_data_path, "%s/data", path);
	}
}
MyDataConfig::~MyDataConfig()
{
	if (_log_path)
		delete _log_path;
	if (_data_path)
		delete _data_path;
}