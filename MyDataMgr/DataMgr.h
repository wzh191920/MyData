#pragma once
#include "windows.h"
#include "datas.pb.h"
#include "LogFilew.h"

struct DataEndArray;
struct DataHead;
struct FileSet;
class DataFileMgr;
class MyDataFun;
class MyDataConfig;
class DataMgr
{
public:
	DataMgr();
	~DataMgr();
	int Init(CLogFile *log, MyDataConfig *config);
	int WriteNewData(int data_id, time_t t, const void* data, int data_length, MyDataFun* function);	
	int ReadNewData(int data_id, time_t &t, void* data, int &data_length);
	int ReadDatasNum(int data_id, time_t time_begin, time_t time_end, int &num);	
	int ReadDatas(int data_id, time_t time_begin, time_t time_end, Datas::ReadDatasReply &reply);	
	int UpdateData(int data_id, time_t t, void*data, int data_length);		
	int WriteOldDatas(int data_id, Datas::WriteOldDatasQuery &query, int &num, MyDataFun* function);	
	int RemoveDatas(Datas::RemoveDatasQuery &query);
	int RemovePoint(int data_id);
	int CreateFile(const char* filename, long long file_size, time_t start_time, time_t end_time);
	int DeleteFile(const char* filename, time_t start_time);
	int AddFile(const char* filename);																
	int GetFilesNum();
	void GetFileInfos(Datas::DataFileInfosReply &fileinfos);
	int ReBuildIndex(const char* filename, time_t start_time);	
	int ReBuildDataFileMgr();
	void StopRunning();
	bool IsRunning();
	static bool _run;
	static SRWLOCK _rwlock;															//¿ØÖÆdatafile_mgr_.mgr_
	static bool CompareDataEnd(const DataEndArray &a1, const DataEndArray &a2);
	inline static DataEndArray *GetDataEnd(DataHead* data_head);
private:
	DataFileMgr *datafile_mgr_;
};

