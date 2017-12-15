// MyDataPlugIn.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "MyDataPlugIn.h"

MYDATA_API void PlugInReadChar(SerializerNoBuf* io, char *data)
{
	*data = io->buf[io->data_pos++];
}

MYDATA_API void PlugInReadShort(SerializerNoBuf* io, short *data)
{
	memcpy(data, io->buf+io->data_pos, sizeof(*data));
	io->data_pos += sizeof(*data);
}

MYDATA_API void PlugInReadInt(SerializerNoBuf* io, int *data)
{
	memcpy(data, io->buf+io->data_pos, sizeof(*data));
	io->data_pos += sizeof(*data);
}

MYDATA_API void PlugInReadLongLong(SerializerNoBuf* io, long long *data)
{
	memcpy(data, io->buf+io->data_pos, sizeof(*data));
	io->data_pos += sizeof(*data);
}

MYDATA_API void PlugInReadFloat(SerializerNoBuf* io, float *data)
{
	memcpy(data, io->buf+io->data_pos, sizeof(*data));
	io->data_pos += sizeof(*data);
}

MYDATA_API void PlugInReadDouble(SerializerNoBuf* io, double *data)
{
	memcpy(data, io->buf+io->data_pos, sizeof(*data));
	io->data_pos += sizeof(*data);
}

MYDATA_API void PlugInReadString(SerializerNoBuf* io, char* data)
{
	int length = strlen(io->buf+io->data_pos)+1;
	memcpy(data, io->buf+io->data_pos, length);
	io->data_pos += length;
}