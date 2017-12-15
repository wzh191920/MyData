#include "StdAfx.h"
#include "PlugIn.h"
#include <algorithm>
#include <fstream>
#include "MyDataCore.h"
#include "StringEdit.h"
using std::vector;
using std::for_each;

PlugIn::~PlugIn(void)
{
	for_each(CloseFuns_.begin(), CloseFuns_.end(), [](CloseFun fun){
		fun();
	});
	for_each(DllModules_.begin(), DllModules_.end(), [](HMODULE module){
		FreeLibrary(module); 
	});
}
void PlugIn::Load(char* path, char *dllname, char *init_name, char *close_name, char *wnew_data_name)
{
	if (dllname[0] == '\0')
		return;
	SetDllDirectory(path);
	HMODULE module = LoadLibrary(dllname);
	if (module == NULL)
	{
		LOG_ERROR_WRITE("加载动态库失败, %s", dllname);
		return;
	}
	LOG_INFO_WRITE("加载动态库%s", dllname);
	WriteNewDataFun wnew_fun = NULL;
	InitFun init_fun = NULL;
	CloseFun close_fun = NULL;
	if (init_name[0])
	{
		init_fun = (InitFun)GetProcAddress(module, init_name);
		if (init_fun)
		{
			if (!init_fun())
			{
				LOG_ERROR_WRITE("初始化动态库失败, %s", dllname);
				FreeLibrary(module);
				return;
			}
		}
		else
		{
			LOG_ERROR_WRITE("获取动态库初始化地址失败, 地址名%s", init_name);
			FreeLibrary(module);
			return;
		}
		LOG_INFO_WRITE("运行动态库函数%s", init_name);
	}
	if (close_name[0])
	{
		close_fun = (CloseFun)GetProcAddress(module, close_name);
		if (close_fun == NULL)
		{
			LOG_ERROR_WRITE("获取动态库结束地址失败, 地址名%s", close_name);
			FreeLibrary(module);
			return;
		}
	}
	if (wnew_data_name[0])
	{
		wnew_fun = (WriteNewDataFun)GetProcAddress(module, wnew_data_name);
		if (wnew_fun == NULL)
		{
			LOG_ERROR_WRITE("获取动态库写数据插件失败, 地址名%s", wnew_data_name);
			FreeLibrary(module);
			return;
		}
	}
	if (close_fun)
	{
		CloseFuns_.push_back(close_fun);
		LOG_INFO_WRITE("加载动态库函数%s", close_name);
	}
	if (wnew_fun)
	{
		WriteNewDataFuns_.push_back(wnew_fun);
		LOG_INFO_WRITE("加载动态库函数%s", wnew_data_name);
	}
	DllModules_.push_back(module);
}
void PlugIn::Init()
{
	char path[MAX_PATH];
	const int buf_size = 512;
	if( !GetModuleFileName( NULL, path, MAX_PATH ) )
	{
		return;
	}
	char *end_pos = strrchr(path, '\\');
	*end_pos = '\0';
	char ini_path[MAX_PATH];
	strcpy(ini_path, path);
	strcat(ini_path, "/plugin.ini");
	std::ifstream filestream(ini_path);
	if (!filestream)
	{
		return;
	}
	char fileline[1024];
	char dllname[MAX_PATH] = {0};
	char init_name[buf_size];
	char close_name[buf_size];
	char wnew_data_name[buf_size];
	while (filestream.good())
	{
		fileline[0] = '\0';
		filestream.getline(fileline, sizeof(fileline));
		char* data = strchr(fileline, '#');
		if (data)
			*data = '\0';
		Replace(fileline, '\t', ' ');
		data = Trim(fileline);
		if (data[0] == '\0')
			continue;
		if (strcmp(data, "%%") == 0)
		{
			Load(path, dllname, init_name, close_name, wnew_data_name);
			*dllname = *init_name = *close_name = *wnew_data_name = '\0';
			continue;
		}
		char* colon = strchr(data, ':');
		if (colon == NULL)
			continue;
		char* v = colon+1;
		char* k = data;
		*colon = '\0';
		v = Trim(v);
		k = Trim(k);
		if (lstrcmpi(k, "Init") == 0)
		{
			strcpy(init_name, v);
		}
		else if (lstrcmpi(k, "Close") == 0)
		{
			strcpy(close_name, v);
		}
		else if (lstrcmpi(k, "WriteNewDataPlugIn") == 0)
		{
			strcpy(wnew_data_name, v);
		}
		else if (lstrcmpi(k, "DllName") == 0)
		{
			strcpy(dllname, v);
		}
	}
	Load(path, dllname, init_name, close_name, wnew_data_name);
}