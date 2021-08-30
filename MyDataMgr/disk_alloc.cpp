#include "stdafx.h"
#include "disk_alloc.h"
#include "io.h"
#define DISK_ALLOC_MAGIC_NUM 0x77880000
int disk_alloc::init(const char *filename, size_t length, size_t expend_length, void (*callback)(void*), void *callback_para)
{
	bool verify = false;
	if (_access(filename, 0) == 0)
	{
		verify = true;
	}
	clean_pos_ = NULL;
	expend_length_ = expend_length;
	callback_ = callback;
	callback_para_ = callback_para;
	int ret = map(filename, length);
	if (ret)
		return ret;
	int *base_addr = (int*)base_addr_;
	if (verify)
	{
		if (*base_addr != DISK_ALLOC_MAGIC_NUM)
			return -1;
		clean_pos_ = base_addr+1;
	}
	else
	{
		*base_addr = DISK_ALLOC_MAGIC_NUM;
		clean_pos_ = base_addr+1;
		*clean_pos_ = sizeof(int)*2;
	}
	return 0;
}

void *disk_alloc::my_alloc(int size)
{
	if (size > 1024)
		return 0;
	if (*clean_pos_ + size > length_)
	{//重新映射
		size_t new_length = length_ + expend_length_;
		if (map(filename_, new_length))
			return 0;
		clean_pos_ = (int*)base_addr_+1;

		if (callback_)
			callback_(callback_para_);
	}
	unsigned char * base_addr = (unsigned char * )base_addr_;
	void* alloc_pos = base_addr + *clean_pos_;
	*clean_pos_ += size;
	return alloc_pos;
}

void *disk_alloc::my_relative_alloc(int size)
{
	if (size > 10240)
		return 0;
	if (*clean_pos_ + size > length_)
	{//重新映射
		size_t new_length = length_ + expend_length_;
		if (map(filename_, new_length))
			return 0;
		clean_pos_ = (int*)base_addr_+1;

		if (callback_)
			callback_(callback_para_);
	}
	void* alloc_pos = (void*)*clean_pos_;
	*clean_pos_ += size;
	return alloc_pos;
}
