#pragma once

#include "MyDataCommon.h"
#include "datas.pb.h"
#include "MyDataMgrCore.h"
#include "LogFilew.h"
#include "MultiSkipListKVDisk.h"
class MyDataConfig;
class DataFileMgr
{
public:
	~DataFileMgr();
	int Init(const char* datafile_mgr_name, MyDataConfig *config);
	int UnInit();
	int CreateFile(const char* filename, long long file_size, time_t start_time, time_t end_time);
	int TransformEmptyFile(time_t start_time);
	//计算数据文件的最新时间
	int GetFileEndTime(const DataFileInfoInner *fileinfo, time_t *end_time);
	//信息在历史文件中冗余保存，便于入列历史文件时，恢复信息于mgr_中
	void UpdateFileInfo(DataFileInfoInner *fileinfo);												
	int DeleteFile(const char* filename, time_t start_time);
	//入列历史文件
	int AddFile(const char* filename);																
	int GetFilesNum();
	void GetFileInfos(Datas::DataFileInfosReply &fileinfos);
	//重建索引和块备份文件
	int ReBuildIndex(const char* filename, time_t start_time);										
	std::pair<time_t, DataFileInfoInner *> GetFile(time_t start_time);
	SkipListKVDisk<time_t, DataFileInfoInner, CommonLowThan<time_t>> &GetMgr() {return mgr_;}
	static const time_t _max_time;
	void AllocBlock(int data_id, FileSet *fileset, time_t t, DataIndex &index, DataHead *&data_head, DataEndArray *&data_end, DataHead *&real_data_head, DataEndArray *&real_data_end);

private:
	MyDataConfig *config_;
	int CreateFile(long long file_size);
	static unsigned int __stdcall AutoCreateFile(void* data); 
	SkipListKVDisk<time_t, DataFileInfoInner, CommonLowThan<time_t>> mgr_;		//key:起始时间
	static int _file_diff;														//区别文件名
};

