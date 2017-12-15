// PlugInTest.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "PlugInTest.h"
#include "MyDataPlugIn.h"

DLL_API bool InitCallBack()
{
	//程序加载时，初始化写这里，返回值决定是否加载成功
	return true;
}
DLL_API bool WriteNewDataCallBack(int table_id, int point_id, time_t t, const char* data)
{//写入实时数据前的处理写这里，返回值决定是否写入数据库

	//功能，如果是表id为1的数据，如果第一个基本类型（即，仪表1）的数是奇数，则不写入数据库
	if (table_id == 1)
	{
		SerializerNoBuf io(data);
		int i = 0;
		PlugInReadInt(&io, &i);
		if (i & 1)
			return false;
	}
	return true;
}
DLL_API void CloseCallBack()
{
	//程序退出时，资源释放的处理写这里
}
