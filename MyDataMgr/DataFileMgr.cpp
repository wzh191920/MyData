#include "stdafx.h"
#include <memory>
#include "process.h"
#include "io.h"
#include "time.h"
#include "DataFileMgr.h"
#include "DataMgr.h"
#include "MyDataError.h"
#include "MyDataConfig.h"
using std::unique_ptr;
using std::pair;

const time_t DataFileMgr::_max_time = 0x7FFFFFFFFFFFFFFF;
int DataFileMgr::_file_diff = 0;

DataFileMgr::~DataFileMgr()
{
	UnInit();
}
int DataFileMgr::Init(const char* datafile_mgr_name, MyDataConfig *config)
{
	int ret = 0;
	char allpath[MAX_PATH];
	config_ = config;
	sprintf(allpath, "%s/%s", config_->_data_path, datafile_mgr_name);
	ret = mgr_.Init(allpath, 0, INDEX_EXPEND_LENGTH);
	if (ret)
	{
		LOG_ERROR_WRITE("DataFileMgr初始化失败,%d", ret);
		return ret;
	}
	SkipListKVDisk<time_t, DataFileInfoInner, CommonLowThan<time_t>>::Iterator iter(&mgr_);
	for (iter.SeekToFirst(); iter.Valid(); iter.Next())
	{
		DataFileInfoInner &info = iter.value();
		if (iter.key() == -1 && info.filename_[0] == '\0')
			continue;
		FileSet* fileset = new FileSet;
		info.file_set_ = fileset;
		sprintf(allpath, "%s/%s", config_->_data_path, info.filename_);
		ret = fileset->daa_.init(allpath);
		if (ret)
		{
			LOG_ERROR_WRITE("初始化%s的数据文件失败", info.filename_);
			return MYDATA_FILEINIT_FAIL;
		}
		ret = fileset->index_mgr_.Init(allpath, 0, INDEX_EXPEND_LENGTH);
		if (ret)
		{
			LOG_ERROR_WRITE("初始化%s的索引文件失败", info.filename_);
			return MYDATA_FILEINIT_FAIL;
		}
		ret = fileset->skd_.Init(allpath, 0, INDEX_EXPEND_LENGTH*10);
		if (ret)
		{
			LOG_ERROR_WRITE("初始化%s的块回收文件失败", info.filename_);
			return MYDATA_FILEINIT_FAIL;
		}
	}
	HANDLE thandle = (HANDLE)_beginthreadex( NULL, 0, AutoCreateFile, this, 0, NULL);
	if (thandle == 0)
	{
		LOG_ERROR_WRITE("数据文件自动创建线程启动失败");
		return -1;
	}
	else
		CloseHandle(thandle);

	return MYDATA_OK;
}
int DataFileMgr::UnInit()
{
	SkipListKVDisk<time_t, DataFileInfoInner, CommonLowThan<time_t>>::Iterator iter(&mgr_);
	for (iter.SeekToFirst(); iter.Valid(); iter.Next())
	{
		if (iter.key() == -1)
			continue;
		DataFileInfoInner &info = iter.value();
		delete info.file_set_;
	}
	mgr_.Close();
	return MYDATA_OK;
}
int DataFileMgr::CreateFile(long long file_size)
{
	int ret = 0;
	const time_t emptytime = -1;//空文件占用-1的key，且不会删除，如果没有空文件，文件名为""
	WLOCKGUARD(DataMgr::_rwlock);
	SkipListKVDisk<time_t, DataFileInfoInner, CommonLowThan<time_t>>::Iterator iter(&mgr_);
	iter.Seek(emptytime);
	time_t k = iter.key();
	if (iter.Valid() && iter.key() == emptytime && iter.value().filename_[0] != '\0')//空文件已经存在，且只能有一个
	{
			return MYDATA_CREATEFILE_FAIL;
	}
	
	DataFileInfoInner info;
	info.file_size_ = file_size;
	info.start_time_ = info.end_time_ = emptytime;
	time_t curtime = time(NULL);
	tm tmtime;
	localtime_s(&tmtime, &curtime);
	sprintf(info.filename_, "%04d%02d%02d_%02d%02d%02d_%d.dat", 
		tmtime.tm_year+1900,
		tmtime.tm_mon + 1,
		tmtime.tm_mday,
		tmtime.tm_hour,
		tmtime.tm_min,
		tmtime.tm_sec,
		++_file_diff);
	char allpath[MAX_PATH];
	sprintf(allpath, "%s/%s", config_->_data_path, info.filename_);

	FileSet *fileset = new FileSet;
	unique_ptr<FileSet> uptr(fileset);
	ret = fileset->daa_.init(allpath);
	if (ret)
	{
		LOG_ERROR_WRITE("初始化%s的数据文件失败", info.filename_);
		return MYDATA_FILEINIT_FAIL;
	}
	ret = fileset->index_mgr_.Init(allpath, 0, INDEX_EXPEND_LENGTH);
	if (ret)
	{
		LOG_ERROR_WRITE("初始化%s的索引文件失败", info.filename_);
		return MYDATA_FILEINIT_FAIL;
	}
	ret = fileset->skd_.Init(allpath, 0, INDEX_EXPEND_LENGTH*10);
	if (ret)
	{
		LOG_ERROR_WRITE("初始化%s的块回收文件失败", info.filename_);
		return MYDATA_FILEINIT_FAIL;
	}
	if (iter.Valid())
	{
		DataFileInfoInner &value = iter.value();
		info.file_set_ = value.file_set_ = fileset;
		value.file_size_ = info.file_size_;
		strcpy(value.filename_, info.filename_);
		UpdateFileInfo(&info);
	}
	else
	{
		info.file_set_ = fileset;
		UpdateFileInfo(&info);
		mgr_.Insert(emptytime, info);
	}
	uptr.release();
	return ret;
}
int DataFileMgr::TransformEmptyFile(time_t start_time)
{
	const time_t emptytime = -1;//空文件占用-1的key，且不会删除，如果没有空文件，文件名为""
	SkipListKVDisk<time_t, DataFileInfoInner, CommonLowThan<time_t>>::Iterator iter(&mgr_);
	iter.Seek(emptytime);
	if (!iter.Valid() || iter.key() != emptytime)
		return MYDATA_NO_EMPTY_FILE;
	
	DataFileInfoInner info = iter.value();
	WLOCKGUARD(info.file_set_->rwlock_);
	if (info.filename_[0] == '\0')
		return MYDATA_NO_EMPTY_FILE;

	iter.value().filename_[0] = '\0';
	info.start_time_ = start_time;
	info.end_time_ = _max_time;//最大值
	WLockGuard w(&DataMgr::_rwlock);
	mgr_.Insert(start_time, info);
	return MYDATA_OK;
}
int DataFileMgr::GetFileEndTime(const DataFileInfoInner *fileinfo, time_t *end_time)
{
	MultiSkipListMgr<time_t, void*, CommonLowThan<time_t>> &index_mgr = fileinfo->file_set_->index_mgr_;
	*end_time = -1;
	index_mgr.ScanElement([=](int t, DataIndex &index){
		DataIndex::Iterator iter(&index);
		iter.SeekToFirst();
		if (iter.Valid())
		{
			if (*end_time < iter.key())
				*end_time = iter.key();
		}
	});
	return MYDATA_OK;
}

int DataFileMgr::DeleteFile(const char* filename, time_t start_time)
{
	WLOCKGUARD(DataMgr::_rwlock);
	SkipListKVDisk<time_t, DataFileInfoInner, CommonLowThan<time_t>>::Iterator iter(&mgr_);
	iter.Seek(start_time);
	if (iter.Valid() && iter.key() == start_time && strcmp(iter.value().filename_, filename)==0)
	{
		DataFileInfoInner &info = iter.value();
		time_t end_time = -1;
		GetFileEndTime(&info, &end_time);
		info.end_time_ = end_time;//删除前设置文件的结束时间
		//UpdateFileInfo(&info);//==需要设置活动文件
		delete info.file_set_;
		mgr_.Erase(start_time);
		return MYDATA_OK;
	}

	return MYDATA_DELETEFILE_FAIL;
}

int DataFileMgr::GetFilesNum()
{
	RLOCKGUARD(DataMgr::_rwlock);
	int size = mgr_.Size();
	if (size > 0)
	{
		SkipListKVDisk<time_t, DataFileInfoInner, CommonLowThan<time_t>>::Iterator iter(&mgr_);
		iter.SeekToLast();
		DataFileInfoInner &emptyfile = iter.value();
		if (emptyfile.start_time_ == -1 && emptyfile.filename_[0] == 0)
		{
			--size;
		}
	}
	return size;
}

void DataFileMgr::GetFileInfos(Datas::DataFileInfosReply &fileinfos)
{
	auto fileinfo_array = fileinfos.mutable_data_file_infos();
	RLOCKGUARD(DataMgr::_rwlock);
	SkipListKVDisk<time_t, DataFileInfoInner, CommonLowThan<time_t>>::Iterator iter(&mgr_);
	for (iter.SeekToFirst(); iter.Valid() ; iter.Next())
	{
		DataFileInfoInner &fileinfo_inner = iter.value();
		if (iter.key() == -1 && fileinfo_inner.filename_[0] == 0)
		{
			break;
		}
		auto fileinfo = fileinfo_array->Add();
		fileinfo->set_start_time(fileinfo_inner.start_time_);
		fileinfo->set_end_time(fileinfo_inner.end_time_);
		fileinfo->set_name(fileinfo_inner.filename_);
		long long filesize = fileinfo_inner.file_set_->daa_.filesize();
		long long clean_pos = fileinfo_inner.file_set_->daa_.get_clean_pos();
		if (filesize < fileinfo_inner.file_size_)
			filesize = fileinfo_inner.file_size_;
		fileinfo->set_size(filesize);
		fileinfo->set_using_rate((clean_pos-fileinfo_inner.file_set_->skd_.Length()*BLOCK_SIZE)*100/filesize);//使用率百分比
	}
}

int DataFileMgr::AddFile(const char* filename)
{
	DataFileInfoInner *info;
	if (strlen(filename) > sizeof(info->filename_) - 1)
		return MYDATA_INVALIDE_FILENAME;
	char allpath[MAX_PATH];
	sprintf(allpath, "%s/%s", config_->_data_path, filename);

	if (_access(allpath, 0) != 0)
		return MYDATA_INVALIDE_PARA;

	int ret;
	const time_t emptytime = -1;
	FileSet *fileset = new FileSet;
	unique_ptr<FileSet> uptr(fileset);
	ret = fileset->daa_.init(allpath);
	if (ret)
	{
		LOG_ERROR_WRITE("初始化%s的数据文件失败", filename);
		return MYDATA_FILEINIT_FAIL;
	}
	info = (DataFileInfoInner*)fileset->daa_.get_clean_pos_end();
	time_t start_time = info->start_time_;
	WLOCKGUARD(DataMgr::_rwlock);
	SkipListKVDisk<time_t, DataFileInfoInner, CommonLowThan<time_t>>::Iterator iter(&mgr_);
	iter.Seek(start_time);
	if (iter.Valid())
	{
		DataFileInfoInner *fileinfo = &iter.value();
		if (info->start_time_ < fileinfo->end_time_ )
			return MYDATA_FILE_CONFLICT;
		if (info->start_time_ == fileinfo->end_time_)
		{
			if (info->start_time_ != emptytime)
				return MYDATA_FILE_CONFLICT;
			else if (fileinfo->filename_ != '\0')
			{//添加空历史文件，但是已存在
				return MYDATA_FILE_CONFLICT;
			}
		}
		
		iter.Prev();
		if (iter.Valid())
		{
			fileinfo = &iter.value();
			if (info->end_time_ >= fileinfo->start_time_)
				return MYDATA_FILE_CONFLICT;
		}
		//历史文件肯定比当前的最新历史文件要旧，已无必要检查
		//iter.SeekToFirst();
		//fileinfo = &iter.value();
		////历史文件不能比当前的最新历史文件还要新，简化复杂情况
		//if (info->end_time_ >= fileinfo->start_time_ && fileinfo->start_time_ != emptytime)
		//	return MYDATA_FILE_CONFLICT;
	}
	//==应通过重建索引完成
	ret = fileset->index_mgr_.Init(allpath, 0, INDEX_EXPEND_LENGTH);
	if (ret)
	{
		LOG_ERROR_WRITE("初始化%s的索引文件失败", filename);
		return MYDATA_FILEINIT_FAIL;
	}
	ret = fileset->skd_.Init(allpath, 0, INDEX_EXPEND_LENGTH*10);
	if (ret)
	{
		LOG_ERROR_WRITE("初始化%s的块回收文件失败", filename);
		return MYDATA_FILEINIT_FAIL;
	}
	//
	info->file_set_ = fileset;
	if (info->start_time_ != -1)
		mgr_.Insert(info->start_time_, *info);
	else
	{//空历史文件
		iter.SeekToLast();
		if (iter.Valid())
		{
			DataFileInfoInner *fileinfo = &iter.value();
			if (fileinfo->start_time_ == emptytime && fileinfo->filename_[0] == '\0')
				*fileinfo = *info;
			else
				return MYDATA_FILE_CONFLICT;
		}
		else
		{
			mgr_.Insert(info->start_time_, *info);
		}
	}
	uptr.release();
	return ret;
}

int DataFileMgr::CreateFile(const char* filename, long long file_size, time_t start_time, time_t end_time)
{
	if (start_time == -1 || end_time == -1 || filename == NULL)//不允许添加空文件
		return MYDATA_INVALIDE_PARA;
	char allpath[MAX_PATH];
	sprintf(allpath, "%s/%s", config_->_data_path, filename);

	if (_access(allpath, 0) == 0)
		return MYDATA_INVALIDE_PARA;
	WLOCKGUARD(DataMgr::_rwlock);
	SkipListKVDisk<time_t, DataFileInfoInner, CommonLowThan<time_t>>::Iterator iter(&mgr_);
	iter.Seek(start_time);
	if (iter.Valid())
	{
		DataFileInfoInner *fileinfo = &iter.value();
		if (start_time <= fileinfo->end_time_)
			return MYDATA_FILE_CONFLICT;
		iter.Prev();
		if (iter.Valid())
		{
			fileinfo = &iter.value();
			if (end_time >= fileinfo->start_time_)
				return MYDATA_FILE_CONFLICT;
		}
	}
	FileSet *fileset = new FileSet;
	unique_ptr<FileSet> uptr(fileset);
	int ret = fileset->daa_.init(allpath);
	if (ret)
	{
		LOG_ERROR_WRITE("初始化%s的数据文件失败", allpath);
		return MYDATA_FILEINIT_FAIL;
	}
	ret = fileset->index_mgr_.Init(allpath, 0, INDEX_EXPEND_LENGTH);
	if (ret)
	{
		LOG_ERROR_WRITE("初始化%s的索引文件失败", allpath);
		return MYDATA_FILEINIT_FAIL;
	}
	ret = fileset->skd_.Init(allpath, 0, INDEX_EXPEND_LENGTH*10);
	if (ret)
	{
		LOG_ERROR_WRITE("初始化%s的块回收文件失败", allpath);
		return MYDATA_FILEINIT_FAIL;
	}
	DataFileInfoInner info;
	info.file_size_ = file_size;
	info.start_time_ = start_time;
	info.end_time_ = end_time;
	if (strlen(filename) >= sizeof(info.filename_))
		return MYDATA_INVALIDE_PARA;
	strcpy(info.filename_, filename);
	info.file_set_ = fileset;
	UpdateFileInfo(&info);
	mgr_.Insert(start_time, info);
	uptr.release();
	return ret;
}

std::pair<time_t, DataFileInfoInner *> DataFileMgr::GetFile(time_t start_time)
{
	SkipListKVDisk<time_t, DataFileInfoInner, CommonLowThan<time_t>>::Iterator iter(&mgr_);
	iter.Seek(start_time);
	if (iter.Valid())
		return pair<time_t, DataFileInfoInner *>(iter.key(), &iter.value());
	
	DataFileInfoInner * d=0;
	return pair<time_t, DataFileInfoInner *>(-1, d);
}
int DataFileMgr::ReBuildIndex(const char* filename, time_t start_time)
{
	int ret = MYDATA_OK;
	RLOCKGUARD(DataMgr::_rwlock);
	SkipListKVDisk<time_t, DataFileInfoInner, CommonLowThan<time_t>>::Iterator iter(&mgr_);
	iter.Seek(start_time);
	if (iter.Valid())
	{
		DataFileInfoInner &info = iter.value();
		if (strcmp(info.filename_, filename) != 0 || info.start_time_ != start_time)
			return MYDATA_NO_FIT_FILE;
		FileSet *fileset = info.file_set_;
		WLOCKGUARD(fileset->rwlock_);
		fileset->index_mgr_.Close();
		fileset->skd_.Close();
		char allpath[MAX_PATH];
		sprintf(allpath, "%s/index_%s", config_->_data_path, filename);
		remove(allpath);
		sprintf(allpath, "%s/indexmgr_%s", config_->_data_path, filename);
		remove(allpath);
		sprintf(allpath, "%s/%s", config_->_data_path, filename);
		ret = fileset->index_mgr_.Init(allpath, 0, INDEX_EXPEND_LENGTH);
		if (ret)
		{
			LOG_ERROR_WRITE("重建索引失败, %d", ret);
			return MYDATA_REBUILD_INDEX_FAIL;
		}
		sprintf(allpath, "%s/sd_%s", config_->_data_path, filename);
		remove(allpath);
		sprintf(allpath, "%s/%s", config_->_data_path, filename);
		ret = fileset->skd_.Init(allpath, 0, INDEX_EXPEND_LENGTH*10);
		if (ret)
		{
			LOG_ERROR_WRITE("重建索引失败, %d", ret);
			return MYDATA_REBUILD_INDEX_FAIL;
		}
		size_t granularity = fileset->daa_.get_granularity();
		long long clean_pos = fileset->daa_.get_clean_pos();
		int map_num = granularity/BLOCK_SIZE;
		DataEndArray *data_end = NULL;
		DataEndArray *real_data_end = NULL;
		DataHead *data_head_element = NULL;
		DataHead *real_data_head_element = NULL;
		DataHead *data_head = (DataHead *)granularity;
		for (int i=1; clean_pos > granularity*i; i++)
		{
			data_head = (DataHead *)(granularity*i);
			DataHead *real_data_head = fileset->daa_.get_absolute_pos(data_head);
			for (int j=0; j<map_num; j++)
			{
				data_head_element = (DataHead *)((unsigned char*)data_head + BLOCK_SIZE*j);
				if (data_head_element >= (DataHead *)clean_pos)
					break;
				real_data_head_element = (DataHead *)((unsigned char*)real_data_head + BLOCK_SIZE*j);
				if (real_data_head_element->dirty_)
				{
					DataIndex index;
					if (fileset->index_mgr_.GetSkipList(real_data_head_element->data_id, &index))
					{
						fileset->index_mgr_.CreateSkipList(real_data_head_element->data_id);
						fileset->index_mgr_.GetSkipList(real_data_head_element->data_id, &index);
					}
					if (real_data_head_element->data_array_size_ == 0)
					{
						real_data_head_element->dirty_ = 0;
						fileset->skd_.Push(data_head_element);
					}
					else
					{
						data_end = DataMgr::GetDataEnd(data_head_element);
						real_data_end = fileset->daa_.get_absolute_pos(data_end);
						void* pos = data_head_element;
						index.Insert(real_data_end->datetime_, pos);
					}
				}
				else
				{
					fileset->skd_.Push(data_head_element);
				}
			}
		}
		return MYDATA_OK;
	}
	else
	{
		return MYDATA_NO_FIT_FILE;
	}
}

void DataFileMgr::UpdateFileInfo(DataFileInfoInner *fileinfo)
{
	WLOCKGUARD(fileinfo->file_set_->rwlock_);
	DataFileInfoInner* info = (DataFileInfoInner *)fileinfo->file_set_->daa_.get_clean_pos_end();
	*info = *fileinfo;
}

unsigned int __stdcall DataFileMgr::AutoCreateFile(void* data)
{
	DataFileMgr *This = (DataFileMgr*)data;
	while (DataMgr::_run)
	{
		This->CreateFile(This->config_->_auto_create_size);
		Sleep(30000);
	}
	return 0;
}

void DataFileMgr::AllocBlock(int data_id, FileSet *fileset, time_t t, DataIndex &index, DataHead *&data_head, DataEndArray *&data_end, DataHead *&real_data_head, DataEndArray *&real_data_end)
{

	void* value;
	DataHead** pdata_head = fileset->skd_.Pop();
	if (pdata_head == NULL)
	{
		value = fileset->daa_.relative_alloc(BLOCK_SIZE);
		data_head = (DataHead*)value;
	}
	else
	{
		value = (void*)(*pdata_head);
		data_head = *pdata_head;
	}
	data_end = DataMgr::GetDataEnd(data_head);
	real_data_head = fileset->daa_.get_absolute_pos(data_head);
	real_data_head->data_id = data_id;
	int data_head_pos = (int)data_head;
	real_data_head->new_data_pos_ = data_head_pos + sizeof(DataHead);
	real_data_head->dirty_ = 1;
	real_data_head->data_array_size_ = 0;
	real_data_end = fileset->daa_.get_absolute_pos(data_end);
	index.Insert(t, value);
}
