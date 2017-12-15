#pragma once
#ifndef __MYDATA_API_H__
#define __MYDATA_API_H__

#include "MyDataCommon.h"
#ifdef __cplusplus
extern "C" {
#endif

#ifdef MYDATA_EXPORTS
#define MYDATA_API extern "C" __declspec(dllexport)
#else
#define MYDATA_API extern "C" __declspec(dllimport)
#endif

/*
 * 功能：连接服务端
 */
MYDATA_API int MyDataConnect(const char *ip, int port, int *handle);

/*
 * 功能：断开服务端连接
 */
MYDATA_API int MyDataDisconnect(int handle);

/*
 * 功能：设置与服务端的通信超时
 */
MYDATA_API void MyDataTimeout(int sec);

/*
* 功能：获取错误码的描述
*/
MYDATA_API const char* MyDataGetErrDesc(int err);

/*
* 功能：添加标签点的类型，最多可以添加16个类型种类
* 参数：type_array，类型种类的数组，数组长度不超过16
		array_size，type_array的数组元素个数
		name，添加的类型的名称
		type_names，type_array元素对应的名称，每个名称以";"隔开，组成的字符串
*/
MYDATA_API int MyDataAddType(int handle, DataType* type_array, int array_size, const char* name, const char* type_names);

/*
* 功能：删除标签点的类型
*/
MYDATA_API int MyDataDeleteType(int handle, int type_id);

/*
* 功能：获取标签点类型的数量，便于调用MyDataGetTypes
*/
MYDATA_API int MyDataGetTypeNum(int handle, int *num);

/*
* 功能：获取指定数量的标签点类型
* 参数：type_infos，数组，数组长度由MyDataGetTypeNum返回的结果确定
		num，输入为type_infos的数组元素个数，输出为获取到的个数
*/
MYDATA_API int MyDataGetTypes(int handle, TypeInfo *type_infos, int *num);

/*
* 功能：获取指定的标签点类型，根据@type_id，得到@type_info
*/
MYDATA_API int MyDataGetTypeByID(int handle, int type_id, TypeInfo *type_info);

/*
* 功能：添加表
* 参数：type_id，表对应的类型
		name，表的名称
*/
MYDATA_API int MyDataAddTable(int handle, int type_id, const char *name);

/*
* 功能：删除表
*/
MYDATA_API int MyDataDeleteTable(int handle, int table_id);

/*
* 功能：获取表数量，便于调用MyDataGetTables
*/
MYDATA_API int MyDataGetTableNum(int handle, int *num);

/*
* 功能：获取所有表信息
* 参数：table_info，输出表信息的数组，数组长度由MyDataGetTables的返回结果决定
		num，输入为table_info的元素个数，输出为获取到的个数
*/
MYDATA_API int MyDataGetTables(int handle, TableInfo *table_info, int *num);

/*
* 功能：获取指定的表信息，根据@table_id，得到@table_infos
*/
MYDATA_API int MyDataGetTableByID(int handle, TableInfo *table_infos, int table_id);

/*
* 功能：添加标签点
* 参数：table_id，标签点所要添加的表
		point_names，输入标签点的名称，不应大于31字符
		num，输入为point_names的数组元素个数，输出为添加成功的个数
*/
MYDATA_API int MyDataAddPoints(int handle, int table_id, char **point_names, int *num);

/*
* 功能：删除标签点
*/
MYDATA_API int MyDataDeletePoint(int handle, int point_id);

/*
* 功能：获取标签点数量，便于调用MyDataSearchPoints
* 参数：table_name查询的表名称，支持sql语句的通配符，如%匹配0或多个字符，_匹配一个字符，为""表示全部查找
		point_name查询的标签点名称，支持sql语句的通配符，如%匹配0或多个字符，_匹配一个字符，为""表示全部查找
		num，输出查找到的数量
*/
MYDATA_API int MyDataSearchPointsNum(int handle, const char * table_name, const char *point_name, int *num);

/*
* 功能：获取标签点数量
* 参数：table_id查询的表id
		point_name查询的标签点名称，支持sql语句的通配符，如%匹配0或多个字符，_匹配一个字符，为""表示全部查找
		num，输出查找到的数量
*/
MYDATA_API int MyDataSearchPointsNumByID(int handle, int table_id, const char *point_name, int *num);

/*
* 功能：获取标签点信息
* 参数：table_name查询的表名称，支持sql语句的通配符，如%匹配0或多个字符，_匹配一个字符，为""表示全部查找
		point_name查询的标签点名称，支持sql语句的通配符，如%匹配0或多个字符，_匹配一个字符，为""表示全部查找
		point_infos，输出表信息的数组
		num，输入为point_infos的元素个数，输出为获取到的元素个数
*/
MYDATA_API int MyDataSearchPoints(int handle, const char * table_name, const char *point_name, PointInfo *point_infos, int *num);

/*
* 功能：获取标签点信息
* 参数：table_id查询的表id
		point_name查询的标签点名称，其中支持sql语句的通配符，如%匹配0或多个字符，_匹配一个字符，为""表示全部查找
		point_infos，输出表信息的数组
		num，输入为point_infos的元素个数，输出为获取到的元素个数
*/
MYDATA_API int MyDataSearchPointsByID(int handle, int table_id, const char *point_name, PointInfo *point_infos, int *num);

/*
* 功能：获取数据文件数量，便于调用GetDataFileInfos
*/
MYDATA_API int GetDataFileNums(int handle, int *num);

/*
* 功能：获取数据文件信息
* 参数：data_file_infos，输出数据文件信息
		num，输入为data_file_infos的元素个数，输出为获取到的元素个数
*/
MYDATA_API int GetDataFileInfos(int handle, DataFileInfo *data_file_infos, int *num);

/*
* 功能：把已经存在的数据文件加入数据库，该数据文件通过MyDataDeleteFile后得到，用于移库
*/
MYDATA_API int MyDataAddFile(int handle, const char *filename);

/*
* 功能：创建指定时间范围的数据文件
* 参数：filename创建的文件名称，不应大于31个字符
		start_time，文件的开始时间，UTC时间
		end_time，文件的结束时间，UTC时间
		filesize，文件的大小，不应小于100MB或大于8GB
*/
MYDATA_API int MyDataCreateFile(int handle, const char *filename, time_t start_time, time_t end_time, long long filesize);

/*
* 功能：删除指定的数据文件
* 参数：filename指定文件的名称
		start_time，指定文件的开始时间，UTC时间
*/
MYDATA_API int MyDataDeleteFile(int handle, const char *filename, time_t start_time);

/*
* 功能：指定文件重建索引，用于移库或频繁增删标签点
*/
MYDATA_API int MyDataRebuildIndex(int handle, const char *filename, time_t start_time);

/*
* 功能：写入标签点最新数据
* 参数：point_ids，输入，数组，标签点的id
		table_ids, 输入，数组，标签点的表id
		times，输入，数组，最新数据的时间戳
		datas，输入，数组，数据的内容
		num，输入为以上参数数组的大小，输出为成功写入数据库的标签点个数
*/
MYDATA_API int WriteNewDatas(int handle, const int *point_ids, const int *table_ids, const time_t *times, const Serializer *datas, int *num);

/*
* 功能：读取标签点最新数据
* 参数：point_ids，输入，数组，标签点的id
		type_ids，输入，数组，标签点对应的类型
		times，输出，数组，最新数据的时间戳
		datas，输出，数组，数据的内容
		num，输入为以上参数数组的大小，输出为成功读取到数据的标签点个数
*/
MYDATA_API int ReadNewDatas(int handle, const int *point_ids, const int *type_ids, time_t *times, Serializer *datas, int *num);

/*
* 功能：标签点指定时间范围的数据总数，便于调用ReadDatas
*/
MYDATA_API int ReadDatasNum(int handle, const int point_id, const time_t time_begin, const time_t time_end, int *num);

/*
* 功能：读取指定标签点指定时间范围的数据
* 参数：point_id，输入，指定的标签点id
		type_id，输入，指定的标签点类型
		time_begin，输入，指定所要读取的数据的开始时间
		time_end，输入，指定所要读取的数据的结束时间
		data_times，输出，读取到的数据的时间戳
		datas，输出，读取到的数据内容
		num，输入为data_times、datas数组的元素个数，输出为成功读取到的数据个数
*/
MYDATA_API int ReadDatas(int handle, const int point_id, const int type_id, const time_t time_begin, const time_t time_end, time_t *data_times, Serializer *datas, int *num);

/*
* 功能：写入指定标签点历史数据
* 参数：point_id，输入，指定的标签点id
		times，输入，所要写入的数据的时间戳
		datas，输入，所要写入的数据的内容
		num，输入为times、datas的元素个数，输出为成功写入的数据个数
*/
MYDATA_API int WriteOldDatas(int handle, const int point_id,  const time_t *times, const Serializer *datas, int *num);

/*
* 功能：删除指定标签点数据
* 参数：point_id，输入，指定的标签点id
		times，输入，所要删除的数据的时间戳
		num，输入，times的元素个数
*/
MYDATA_API int RemoveDatas(int handle, const int point_id,  const time_t *times, int num);

//序列化与反序列化，数据总长度应在2048字节以内，向序列化器写入或读取数据的种类，由指定类型决定
/*
* 功能：获取序列化器，用于按照指定类型序列化或反序列化数据
* 参数：type_id，输入，序列化器的类型id
		sync，输入，true:每次都从数据库获取类型信息，一般为false:只有第一次从数据库获取类型信息
		io，输出，初始化后的序列化器
*/
MYDATA_API int MakeSerializer(int handle, int type_id, bool sync, Serializer *io);

/*
* 功能：向序列化器写入char数据，如果不符合调用MakeSerializer的标签点类型@type_id要求，会返回MYDATA_SERIAL_FAIL错误
*/
MYDATA_API int WriteChar(Serializer* io, char data);

/*
* 功能：向序列化器写入short数据，如果不符合调用MakeSerializer的标签点类型@type_id要求，会返回MYDATA_SERIAL_FAIL错误
*/
MYDATA_API int WriteShort(Serializer* io, short data);

/*
* 功能：向序列化器写入int数据，如果不符合调用MakeSerializer的标签点类型@type_id要求，会返回MYDATA_SERIAL_FAIL错误
*/
MYDATA_API int WriteInt(Serializer* io, int data);

/*
* 功能：向序列化器写入long long数据，如果不符合调用MakeSerializer的标签点类型@type_id要求，会返回MYDATA_SERIAL_FAIL错误
*/
MYDATA_API int WriteLongLong(Serializer* io, long long data);

/*
* 功能：向序列化器写入float数据，如果不符合调用MakeSerializer的标签点类型@type_id要求，会返回MYDATA_SERIAL_FAIL错误
*/
MYDATA_API int WriteFloat(Serializer* io, float data);

/*
* 功能：向序列化器写入double数据，如果不符合调用MakeSerializer的标签点类型@type_id要求，会返回MYDATA_SERIAL_FAIL错误
*/
MYDATA_API int WriteDouble(Serializer* io, double data);

/*
* 功能：向序列化器写入string数据，如果不符合调用MakeSerializer的标签点类型@type_id要求，会返回MYDATA_SERIAL_FAIL错误
* 参数：data数据的结束位置以'\0'为准
*/
MYDATA_API int WriteString(Serializer* io, char* data);

/*
* 功能：向序列化器读取char数据，如果不符合调用MakeSerializer的标签点类型@type_id要求，会返回MYDATA_SERIAL_FAIL错误
*/
MYDATA_API int ReadChar(Serializer* io, char *data);

/*
* 功能：向序列化器读取short数据，如果不符合调用MakeSerializer的标签点类型@type_id要求，会返回MYDATA_SERIAL_FAIL错误
*/
MYDATA_API int ReadShort(Serializer* io, short *data);

/*
* 功能：向序列化器读取int数据，如果不符合调用MakeSerializer的标签点类型@type_id要求，会返回MYDATA_SERIAL_FAIL错误
*/
MYDATA_API int ReadInt(Serializer* io, int *data);

/*
* 功能：向序列化器读取long long数据，如果不符合调用MakeSerializer的标签点类型@type_id要求，会返回MYDATA_SERIAL_FAIL错误
*/
MYDATA_API int ReadLongLong(Serializer* io, long long *data);

/*
* 功能：向序列化器读取float数据，如果不符合调用MakeSerializer的标签点类型@type_id要求，会返回MYDATA_SERIAL_FAIL错误
*/
MYDATA_API int ReadFloat(Serializer* io, float *data);

/*
* 功能：向序列化器读取double数据，如果不符合调用MakeSerializer的标签点类型@type_id要求，会返回MYDATA_SERIAL_FAIL错误
*/
MYDATA_API int ReadDouble(Serializer* io, double *data);

/*
* 功能：向序列化器读取string数据，如果不符合调用MakeSerializer的标签点类型@type_id要求，会返回MYDATA_SERIAL_FAIL错误
*/
MYDATA_API int ReadString(Serializer* io, char* data);

/*
* 功能：获取序列化后的数据，MyDataAPI.dll自己使用
*/
MYDATA_API const void* GetSerializeBuf(const Serializer* io, int *length);

#ifdef __cplusplus
}
#endif
#endif//__MYDATA_API_H__
