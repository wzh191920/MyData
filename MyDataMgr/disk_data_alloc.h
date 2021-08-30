#pragma once
#include "mymmapw.h"

//数据文件的内存分配管理
//内存结构
//MAGIC|clean_pos_|DataFileInfo|...|DATA
//|-----------64k(粒度)------------|
#define  MYDATA_ALLOC_LENGTH	 1024*10240	//一次性分配10MB

class disk_data_alloc
{
public:
	int init(const char *filename);
	const char* filename();
	long long filesize();
	void* get_clean_pos_end() {return clean_pos_+1;};
	long long get_clean_pos() {return *clean_pos_;}
	void *relative_alloc(int size);
	template <typename T>
	T * get_absolute_pos(T * relative_pos)
	{
		long long offset = (long long)relative_pos;
		if (offset >= real_offset_ && offset < real_offset_ + MYDATA_ALLOC_LENGTH)//注意左和右的边界
			return (T*)((unsigned char*)body_.addr()+offset-real_offset_);
		real_offset_ = offset/granularity_*granularity_;
		size_t length = MYDATA_ALLOC_LENGTH;
		int ret = body_.map(length, 4, real_offset_);
		if (ret)
			return 0;
		return (T*)((unsigned char*)body_.addr()+offset-real_offset_);
	}
	size_t get_granularity() {return granularity_;}
private:
	long long *clean_pos_;	//未被分配的位置
	mymmap head_;
	mymmap body_;
	static size_t granularity_;
	long long real_offset_;//内存偏移映射的偏移量
};

