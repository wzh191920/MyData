#pragma once
//MyData插件，函数名可以自定义，但是参数和返回值参照PlugIn.h中的要求，不需要三个回调函数都实现
//插件的配置参照：插件使用说明.txt

#ifdef __cplusplus
extern "C" {
#endif

#define DLL_API extern "C" __declspec(dllexport)

//插件加载时的初始化回调
//返回值为true，插件加载成功，否则加载失败
DLL_API bool InitCallBack();

//实时数据写入前的回调，可以用于判断是否符合写入条件，也可以把数据发往其他程序
//返回值为true，把数据写入数据库，否则丢弃数据
DLL_API bool WriteNewDataCallBack(int table_id, int point_id, time_t t, const char* data);

//程序结束时，插件释放资源的回调函数
DLL_API void CloseCallBack();

#ifdef __cplusplus
}
#endif