#pragma once
#include <vector>
#include "Windows.h"
//插件加载和卸载的键名
//DllName:插件的动态库名称
//Init:对应于InitFun
//Close:对应于CloseFun
//WriteNewDataPlugIn:对应于WriteNewDataFun

class PlugIn
{
public:
	//插件加载时的初始化回调
	typedef bool (WINAPI *InitFun)();
	//实时数据写入前的回调，可以用于判断是否符合写入条件，也可以把数据发往其他程序
	typedef bool (WINAPI *WriteNewDataFun)(int table_id, int point_id, time_t t, const char* data);
	//程序结束插件释放资源的回调函数
	typedef void (WINAPI *CloseFun)();

	void Init();
	~PlugIn(void);

	std::vector<HMODULE> DllModules_;
	std::vector<CloseFun> CloseFuns_;
	std::vector<WriteNewDataFun> WriteNewDataFuns_;
private:
	void Load(char* path, char *dllname, char *init_name, char *close_name, char *wnew_data_name);
};

