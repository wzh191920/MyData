#pragma once
#include "disk_alloc.h"
#include <vector>
template<typename T>
class StackDisk
{
public:
	StackDisk(){};
	int Init(const char* filename, int length, int expend_length);
	int Push(T &element);
	T* Pop();
	int Close();
	int Length();
private:
	int *length_;		//栈中数据数组长度	
	int *capacity_;		//栈中数组容量
	T* array_;
	disk_alloc da_;
	const static int _grow_size = 1024;//栈容量增长长度
};

template<typename T>
int StackDisk<typename T>::Init(const char* filename, int length, int expend_length)
{
	char fullfilename[MAX_PATH] = {0};
	const char *mid_pos = strrchr(filename, '/');
	if (mid_pos == NULL)
		mid_pos = filename;
	else
		mid_pos++;
	strncpy(fullfilename, filename, mid_pos-filename);
	strcat(fullfilename, "sd_");
	strcat(fullfilename, mid_pos);
	int ret = da_.init(fullfilename, length, expend_length);
	if (ret) return ret;
	if (da_.isclean())
	{
		length_ = (int*)da_.my_relative_alloc(sizeof(int));
		int *length = da_.get_absolute_pos(length_);
		*length = 0;
		capacity_ = (int*)da_.my_relative_alloc(sizeof(int));
		int *capacity = da_.get_absolute_pos(capacity_);
		*capacity = _grow_size;
		array_ = (T*)da_.my_relative_alloc(sizeof(T)*_grow_size);
	}
	else
	{
		length_ = (int*)(sizeof(int)*2); //跳过disk_alloc的头部
		capacity_ = (int*)(sizeof(int)*3);
		array_ = (T*)(sizeof(int)*4);
	}
	return ret;
}

template<typename T>
int StackDisk<typename T>::Push(T &element)
{
	int *length = da_.get_absolute_pos(length_);
	int *capacity = da_.get_absolute_pos(capacity_);
	if (*length >= *capacity)
	{
		if (da_.my_relative_alloc(sizeof(T)*_grow_size) == 0)
			return -1;
		length = da_.get_absolute_pos(length_);
		capacity = da_.get_absolute_pos(capacity_);
		*capacity += _grow_size;
	}
	T* array = da_.get_absolute_pos(array_);
	array[(*length)++] = element;
	return 0;
}

template<typename T>
T* StackDisk<typename T>::Pop()
{
	int *length = da_.get_absolute_pos(length_);
	if (*length <= 0)
		return NULL;
	T* array = da_.get_absolute_pos(array_);
	return array + --*length;
}

template<typename T>
int StackDisk<typename T>::Close()
{
	return da_.close();
}

template<typename T>
int StackDisk<typename T>::Length()
{
	int *length = da_.get_absolute_pos(length_);
	return *length;
}