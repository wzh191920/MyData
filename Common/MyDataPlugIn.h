#pragma once
#include "MyDataCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef MYDATA_EXPORTS
#define MYDATA_API extern "C" __declspec(dllexport)
#else
#define MYDATA_API extern "C" __declspec(dllimport)
#endif

//==============================================
//				以下API都带有前缀PlugIn，应由插件使用，不需要与MyData建立连接
//==============================================

//序列化与反序列化，数据总长度应在2048字节以内，向序列化器写入或读取数据的种类，由指定类型决定

/*
* 功能：向序列化器读取char数据，不检查是否符合标签点类型要求
*/
MYDATA_API void PlugInReadChar(SerializerNoBuf* io, char *data);

/*
* 功能：向序列化器读取short数据，不检查是否符合标签点类型要求
*/
MYDATA_API void PlugInReadShort(SerializerNoBuf* io, short *data);

/*
* 功能：向序列化器读取int数据，不检查是否符合标签点类型要求
*/
MYDATA_API void PlugInReadInt(SerializerNoBuf* io, int *data);

/*
* 功能：向序列化器读取long long数据，不检查是否符合标签点类型要求
*/
MYDATA_API void PlugInReadLongLong(SerializerNoBuf* io, long long *data);

/*
* 功能：向序列化器读取float数据，不检查是否符合标签点类型要求
*/
MYDATA_API void PlugInReadFloat(SerializerNoBuf* io, float *data);

/*
* 功能：向序列化器读取double数据，不检查是否符合标签点类型要求
*/
MYDATA_API void PlugInReadDouble(SerializerNoBuf* io, double *data);

/*
* 功能：向序列化器读取string数据，不检查是否符合标签点类型要求
*/
MYDATA_API void PlugInReadString(SerializerNoBuf* io, char* data);

#ifdef __cplusplus
}
#endif