#pragma once

enum DataType
{
	DataChar = 1,
	DataInt16,
	DataInt32,
	DataInt64,
	DataFloat,
	DataDouble,
	DataString
};

#define IP_LENGTH 16
#define NAME_SIZE 32
#define TYPE_SIZE 16
#define MAX_DATA_SIZE 2048		//数据的最大大小
#define TYPE_NAME 512

struct TypeInfo
{
	int id;
	char type[TYPE_SIZE];		//存放DataType，表示类型，超出使用的类型数量时，设为0
	char name[NAME_SIZE];
	char type_names[TYPE_NAME];	//每个类型对应的名称，名称之间用;隔开
};

struct TableInfo
{
	int id;
	int type_id;
	char name[NAME_SIZE];
};

struct PointInfo
{
	int id;
	int table_id;
	char name[NAME_SIZE];
};
//序列化器，读写数据时使用
struct Serializer
{
	char type[TYPE_SIZE];
	int offset;				//type检测的位置
	char buf[MAX_DATA_SIZE];
	int data_pos;			//buf写入起始位置
};
//插件使用的序列化器
struct SerializerNoBuf
{
	explicit SerializerNoBuf(const char* data):buf(data),data_pos(0){}
	const char *buf;
	int data_pos;			//buf写入起始位置
};

struct DataFileInfo
{
	time_t start_time;		
	time_t end_time;		
	long long file_size;	
	float using_rate;
	char filename[NAME_SIZE];		
};

//从小到大
template<typename T>
class CommonGreatThan
{
public:
	int operator()(T a, T b)const
	{
		if (a > b) return 1;
		if (a < b) return -1;
		return 0;
	}
};
//从大到小
template<typename T>
class CommonLowThan
{
public:
	int operator()(T a, T b)const
	{
		if (a < b) return 1;
		if (a > b) return -1;
		return 0;
	}
};

class MyDataFun
{
public:
	virtual int MyDataCallBack(void*) = 0;
	void* data_;
};