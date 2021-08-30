#pragma once
#include "MyDataCommon.h"
#include "MultiSkipListKVDisk.h"
#include "disk_data_alloc.h"
#include "StackDisk.h"
#define  BLOCK_SIZE  8192		//块大小
#define  MAX_PATH 260
//数据块头
struct DataHead
{
	short dirty_;			//已使用的块，1为已使用，0为未使用
	short data_array_size_;	//DataEndArray的长度
	int new_data_pos_;		//未使用的相对起始地址
	int data_id;			//标签点id
};
//数据块尾
struct DataEndArray
{
	int datetime_;			//记录的时间
	int data_pos_;			//记录的相对地址
};

struct FileSet
{
	FileSet()
	{
		InitializeSRWLock(&rwlock_);
	}
	disk_data_alloc daa_;
	MultiSkipListMgr<time_t, void*, CommonLowThan<time_t>> index_mgr_;	//key为最早的数据的时间
	StackDisk<DataHead*> skd_;											//保存删除的块，以备重用
	SRWLOCK rwlock_;													
};
typedef MultiSkipListKVDisk<time_t, void*, CommonLowThan<time_t>> DataIndex;

//数据文件信息
struct DataFileInfoInner : public SerBase
{
	time_t start_time_;		//历史文件开始时间，空历史文件为-1
	time_t end_time_;		//历史文件当前结束时间，没有为-1
	long long file_size_;	//初始文件大小
	FileSet *file_set_;		//临时变量
	char filename_[32];		//为文件的创建时间，没有空文件的文件名为""
};
//任务查询数量
#define INDEX_EXPEND_LENGTH 1024	//索引文件增加长度
#define DATAS_QUERY_BUFFER 100000	//数据查询最大缓存数


struct RLockGuard
{
	RLockGuard(SRWLOCK* lock):lock_(lock)
	{
		AcquireSRWLockShared(lock_);
	}
	~RLockGuard()
	{
		ReleaseSRWLockShared(lock_);
	}
	SRWLOCK *lock_;
};
#define RLOCKGUARD(lock) RLockGuard rg(&lock)
struct WLockGuard
{
	WLockGuard(SRWLOCK* lock):lock_(lock)
	{
		AcquireSRWLockExclusive(lock_);
	}
	~WLockGuard()
	{
		ReleaseSRWLockExclusive(lock_);
	}
	SRWLOCK *lock_;
};
#define WLOCKGUARD(lock) WLockGuard wg(&lock)