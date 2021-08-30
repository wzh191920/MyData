#include "stdafx.h"
#include "disk_data_alloc.h"
#include "io.h"

#define DISK_DATA_ALLOC_MAGIC_NUM 0x55660000

size_t disk_data_alloc::granularity_ = 0;
int disk_data_alloc::init(const char *filename)
{
	bool verify = false;
	int ret = 0;
	real_offset_ = 0xFFFFFFFFFFFFFF;//用于第一次获取绝对地址必须重新分配
	if (_access(filename, 0) == 0)
	{
		verify = true;
	}
	
	if (granularity_ == 0) 
	{
		SYSTEM_INFO sinfo;
		GetSystemInfo(&sinfo);
		granularity_= sinfo.dwAllocationGranularity;
	}
	size_t head_size = 1024;
	ret = head_.map(filename, head_size);
	if (ret) 
		return ret;
	long long *base_addr = (long long*)head_.addr();
	if (verify)
	{
		if (*base_addr != DISK_DATA_ALLOC_MAGIC_NUM)
			return -1;
		clean_pos_ = base_addr+1;
	}
	else
	{
		*base_addr = DISK_DATA_ALLOC_MAGIC_NUM;
		clean_pos_ = base_addr+1;
		*clean_pos_ = granularity_;//保证数据块内存地址是粒度的整数倍
	}
	ret = body_.map(filename, granularity_);
	return ret;
}

const char* disk_data_alloc::filename()
{
	return body_.filename();
}

void *disk_data_alloc::relative_alloc(int size)
{
	if (size > 10240)
		return 0;
	void* alloc_pos = (void*)*clean_pos_;
	*clean_pos_ += size;
	return alloc_pos;
}

long long disk_data_alloc::filesize()
{
	long long fs;
	if (head_.get_filesize(&fs))
		return fs;
	return 0;
}