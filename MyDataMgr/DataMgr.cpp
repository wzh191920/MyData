#include "stdafx.h"
#include <algorithm>
#include "process.h"
#include "DataMgr.h"
#include "DataFileMgr.h"
#include "MyDataMgrCore.h"
#include "MyDataError.h"
#include "MyDataCommon.h"
using std::pair;
using std::vector;
using std::binary_search;
SRWLOCK DataMgr::_rwlock;
bool DataMgr::_run;
DataMgr::DataMgr()
{
	datafile_mgr_ = NULL;
}
DataMgr::~DataMgr()
{
	if (datafile_mgr_)
		delete datafile_mgr_;
}
int DataMgr::Init(CLogFile *log, MyDataConfig *config)
{
	_run = true;
	CLogFile::Instance(log);
	InitializeSRWLock(&_rwlock);
	const char* filename = "datafile_mgr.dat";
	datafile_mgr_ = new DataFileMgr;
	int ret = datafile_mgr_->Init(filename, config);	
	return ret;
}

bool DataMgr::CompareDataEnd(const DataEndArray &a1, const DataEndArray &a2)
{
	return a1.datetime_ > a2.datetime_;
}
int DataMgr::WriteNewData(int data_id, time_t t, const void* data, int data_length, MyDataFun* function)
{
	int ret = MYDATA_OK;
	RLOCKGUARD(_rwlock);
	pair<time_t, DataFileInfoInner *> datafile = datafile_mgr_->GetFile(t);
	if (datafile.second == NULL)
		return MYDATA_NO_EMPTY_FILE;
	if (datafile.first == -1)//空文件
	{
		if (datafile_mgr_->GetFilesNum() > 1)//写的时间太老，找到了空文件
			return MYDATA_NO_FIT_FILE;
		ReleaseSRWLockShared(&_rwlock);
		ret = datafile_mgr_->TransformEmptyFile(t);
		if (ret)
		{
			AcquireSRWLockShared(&_rwlock);
			return ret;
		}
		AcquireSRWLockShared(&_rwlock);
		datafile = datafile_mgr_->GetFile(t);
		datafile_mgr_->UpdateFileInfo(datafile.second);
	}
	if (datafile.second->end_time_ < t)
	{//可能文件已被删除
		return MYDATA_NO_FIT_FILE;
	}
	FileSet *file_set = datafile.second->file_set_;
	//判断文件是否已写满
	long long clean_pos = file_set->daa_.get_clean_pos();
	if (clean_pos > datafile.second->file_size_ && datafile.second->end_time_ == datafile_mgr_->_max_time)//最新的文件需要判断
	{//文件已写满
		time_t end_time = -1;
		datafile_mgr_->GetFileEndTime(datafile.second, &end_time);
		if (end_time == -1)
		{
			LOG_WARNING_WRITE("文件结束时间是-1");
			end_time = t-1;
		}
		time_t start_time = datafile.first;
		ReleaseSRWLockShared(&_rwlock);
		ret = datafile_mgr_->TransformEmptyFile(end_time+1);
		if (ret)
		{
			AcquireSRWLockShared(&_rwlock);
			return ret;
		}
		AcquireSRWLockShared(&_rwlock);
		datafile = datafile_mgr_->GetFile(start_time);
		if (datafile.first == start_time)
		{
			datafile.second->end_time_ = end_time;//修改原来文件的结束时间
			datafile_mgr_->UpdateFileInfo(datafile.second);
		}
		datafile = datafile_mgr_->GetFile(t);//获取到的不一定是最新的数据文件
		file_set = datafile.second->file_set_;
		datafile_mgr_->UpdateFileInfo(datafile.second);
	}
	//从索引找数据块
	DataIndex index;
	WLOCKGUARD(file_set->rwlock_);
	if (file_set->index_mgr_.GetSkipList(data_id, &index))
	{
		if (function->MyDataCallBack(&data_id))
		{
			file_set->index_mgr_.CreateSkipList(data_id);
			if (file_set->index_mgr_.GetSkipList(data_id, &index))
				return MYDATA_POINT_NOT_EXIST;
		}
		else
		{
			return MYDATA_POINT_NOT_EXIST;
		}
	}
	DataIndex::Iterator iter(&index);
	iter.Seek(t);
	DataHead* data_head = NULL;
	DataEndArray *data_end = NULL;
	DataHead *real_data_head = NULL;
	DataEndArray *real_data_end = NULL;
	if (iter.Valid())
	{
		data_head = (DataHead*)iter.value();
		//数据是否可以插入
		data_end = GetDataEnd(data_head);
		real_data_head = (DataHead*)file_set->daa_.get_absolute_pos(data_head);
		real_data_end = file_set->daa_.get_absolute_pos(data_end);
		if (real_data_head->data_array_size_ > 0)
		{
			DataEndArray *data_end_begin = real_data_end - real_data_head->data_array_size_ + 1;
			if (t < data_end_begin->datetime_)
			{//数据不是最新的
				return MYDATA_DATA_NOT_NEW;
			}
			DataEndArray end_value;
			end_value.datetime_ = t;
			if (binary_search(data_end_begin, real_data_end + 1, end_value, CompareDataEnd))
			{//数据的时间戳已存在
				return MYDATA_DATA_TIME_EXIST;
			}
		}
		//数据块空间是否足够
		unsigned char* end_pos = (unsigned char*)(data_end - real_data_head->data_array_size_);
		if (end_pos < (unsigned char*)real_data_head->new_data_pos_ + data_length)
		{
			datafile_mgr_->AllocBlock(data_id, file_set, t, index, data_head, data_end, real_data_head, real_data_end);
		}
	}
	else
	{
		datafile_mgr_->AllocBlock(data_id, file_set, t, index, data_head, data_end, real_data_head, real_data_end);
	}
	//写数据
	void* real_new_data_pos = file_set->daa_.get_absolute_pos((void*)(real_data_head->new_data_pos_));
	memcpy(real_new_data_pos, data, data_length);
	DataEndArray *new_data_end_begin = real_data_end - real_data_head->data_array_size_;
	new_data_end_begin->datetime_ = t;
	new_data_end_begin->data_pos_ = real_data_head->new_data_pos_;
	real_data_head->new_data_pos_ += data_length;
	real_data_head->data_array_size_++;
	return ret;
}
DataEndArray *DataMgr::GetDataEnd(DataHead* data_head)
{
	return (DataEndArray *)((unsigned char*)data_head + BLOCK_SIZE - sizeof(DataEndArray));
}

int DataMgr::UpdateData(int data_id, time_t t, void*data, int data_length)
{
	int ret = MYDATA_OK;
	void* real_data = NULL;
	int old_data_length = 0;
	RLOCKGUARD(_rwlock);
	pair<time_t, DataFileInfoInner *> datafile = datafile_mgr_->GetFile(t);
	if (datafile.first == -1)
		return MYDATA_DATA_NOT_EXIST;

	FileSet *file_set = datafile.second->file_set_;
	WLOCKGUARD(file_set->rwlock_);
	DataIndex index;
	if (file_set->index_mgr_.GetSkipList(data_id, &index))
		return MYDATA_POINT_NOT_EXIST;
	DataIndex::Iterator iter(&index);
	iter.Seek(t);
	if (iter.Valid())
	{
		DataHead* data_head = NULL;
		DataEndArray *data_end = NULL;
		DataHead *real_data_head = NULL;
		DataEndArray *real_data_end = NULL;

		data_head = (DataHead*)iter.value();
		data_end = GetDataEnd(data_head);
		real_data_head = (DataHead*)file_set->daa_.get_absolute_pos(data_head);
		real_data_end = file_set->daa_.get_absolute_pos(data_end);
		DataEndArray *data_end_begin = real_data_end - real_data_head->data_array_size_ + 1;
		DataEndArray end_value;
		end_value.datetime_ = t;

		DataEndArray* find_value = std::lower_bound(data_end_begin, real_data_end + 1, end_value, CompareDataEnd);
		if (find_value == real_data_end + 1 || find_value->datetime_ != t)
			return MYDATA_DATA_NOT_EXIST;
		real_data = file_set->daa_.get_absolute_pos((void*)(find_value->data_pos_));
		//计算数据的长度
		if (real_data_end - find_value == real_data_head->data_array_size_ - 1)
		{//数组的最后一个
			old_data_length = real_data_head->new_data_pos_ - find_value->data_pos_;
		}
		else
		{
			old_data_length = (find_value - 1)->data_pos_ - find_value->data_pos_;
		}
	}
	else
	{
		return MYDATA_DATA_NOT_EXIST;
	}
	
	if (old_data_length < data_length)
		return MYDATA_DATA_TOO_LONG;
	memcpy(real_data, data, data_length);
	return ret;
}

int DataMgr::WriteOldDatas(int data_id, Datas::WriteOldDatasQuery &query, int &num, MyDataFun* function)
{
	int ret = MYDATA_OK;
	WLOCKGUARD(_rwlock);
	auto &mgr = datafile_mgr_->GetMgr();
	SkipListKVDisk<time_t, DataFileInfoInner, CommonLowThan<time_t>>::Iterator iter(&mgr);
	auto array = query.data_infos();
	num = 0;
	for (auto data_iter = array.begin(); data_iter != array.end(); ++data_iter)
	{
		time_t t = data_iter->time_stamp();
		iter.Seek(t);
		if (iter.Valid())
		{
			DataFileInfoInner &fileinfo = iter.value();
			if (t > fileinfo.end_time_ || t < fileinfo.start_time_)
			{//没有合适的数据文件
				continue;
			}
			FileSet *file_set = fileinfo.file_set_;
			WLOCKGUARD(file_set->rwlock_);
			DataIndex index;
			if (file_set->index_mgr_.GetSkipList(data_id, &index))
			{//合适的数据文件里面没有指定的标签点id
				if (function->MyDataCallBack(&data_id))
				{
					file_set->index_mgr_.CreateSkipList(data_id);
					if (file_set->index_mgr_.GetSkipList(data_id, &index))
						continue;
				}
				else
				{
					continue;
				}
			}
			DataIndex::Iterator index_iter(&index);
			time_t begin_t = data_iter->time_stamp();
			for (;data_iter != array.end(); ++data_iter)
			{
				t = data_iter->time_stamp();
				if (t <= fileinfo.end_time_)
				{
					DataHead* data_head = NULL;
					DataEndArray *data_end = NULL;
					DataHead *real_data_head = NULL;
					DataEndArray *real_data_end = NULL;
					index_iter.Seek(t);
					if (index_iter.Valid())
					{
						data_head = (DataHead*)index_iter.value();
						data_end = GetDataEnd(data_head);
						real_data_head = (DataHead*)file_set->daa_.get_absolute_pos(data_head);
						real_data_end = file_set->daa_.get_absolute_pos(data_end);
						DataEndArray *data_end_begin = real_data_end - real_data_head->data_array_size_ + 1;
						if (t > data_end_begin->datetime_)//追加
						{
							//数据块空间是否足够
							unsigned char* end_pos = (unsigned char*)(data_end - real_data_head->data_array_size_);
							if (end_pos < (unsigned char*)real_data_head->new_data_pos_ + data_iter->data().length())
							{//剩余空间不够，新建块
								datafile_mgr_->AllocBlock(data_id, file_set, t, index, data_head, data_end, real_data_head, real_data_end);
							}
							void* real_new_data_pos = file_set->daa_.get_absolute_pos((void*)(real_data_head->new_data_pos_));
							memcpy(real_new_data_pos, data_iter->data().c_str(), data_iter->data().length());
							DataEndArray *new_data_end_begin = real_data_end - real_data_head->data_array_size_;
							new_data_end_begin->datetime_ = t;
							new_data_end_begin->data_pos_ = real_data_head->new_data_pos_;
							real_data_head->new_data_pos_ += data_iter->data().length();
							real_data_head->data_array_size_++;
							num++;
						}
						else//插入
						{
							DataEndArray end_value;
							end_value.datetime_ = t;
							DataEndArray* find_value = std::lower_bound(data_end_begin, real_data_end + 1, end_value, CompareDataEnd);
							if (find_value->datetime_ == t)
							{//时间戳已存在
								continue;
							}
							int move_element_length = find_value - data_end_begin;
							find_value--;
							int move_alldata_length = real_data_head->new_data_pos_-find_value->data_pos_;
							//原数据块数据去除
							real_data_head->new_data_pos_ = find_value->data_pos_;
							real_data_head->data_array_size_ -= move_element_length;
							//缓存要移动的数据
							unsigned char move_buf[BLOCK_SIZE];
							void* move_begin = file_set->daa_.get_absolute_pos((void*)find_value->data_pos_);
							memcpy(move_buf, move_begin, move_alldata_length);
							//重定向数据位置
							for (int i=0; i<move_element_length; i++)
							{
								data_end_begin[i].data_pos_ -= find_value->data_pos_;
							}
							memcpy(move_buf+move_alldata_length, data_end_begin, move_element_length*sizeof(*data_end_begin));
							DataEndArray *move_array = (DataEndArray *)(move_buf+move_alldata_length);
							if (real_data_head->data_array_size_ == 0)//插入一个曾经被删除的第一个数
							{
								index.Erase(index_iter.key());
								file_set->skd_.Push(data_head);
							}
							//分配新数据块
							datafile_mgr_->AllocBlock(data_id, file_set, t, index, data_head, data_end, real_data_head, real_data_end);
							//插入数据
							void* real_new_data_pos = file_set->daa_.get_absolute_pos((void*)(real_data_head->new_data_pos_));
							memcpy(real_new_data_pos, data_iter->data().c_str(), data_iter->data().length());
							DataEndArray *new_data_end_begin = real_data_end - real_data_head->data_array_size_;
							new_data_end_begin->datetime_ = t;
							new_data_end_begin->data_pos_ = real_data_head->new_data_pos_;
							real_data_head->new_data_pos_ += data_iter->data().length();
							real_data_head->data_array_size_++;
							//分配新数据块
							datafile_mgr_->AllocBlock(data_id, file_set, move_array[move_element_length-1].datetime_, index, data_head, data_end, real_data_head, real_data_end);
							//移动数据
							new_data_end_begin = real_data_end - real_data_head->data_array_size_;

							int move_data_length;
							unsigned char* end_pos;
							for (int i=move_element_length-1; i>=0; i--)
							{
								if (i > 0)
									move_data_length = move_array[i-1].data_pos_ - move_array[i].data_pos_;
								else
									move_data_length = move_alldata_length - move_array[i].data_pos_;	
								real_new_data_pos = file_set->daa_.get_absolute_pos((void*)(real_data_head->new_data_pos_));
								memcpy(real_new_data_pos, move_buf+move_array[i].data_pos_, move_data_length);

								new_data_end_begin->datetime_ = move_array[i].datetime_;
								new_data_end_begin->data_pos_ = real_data_head->new_data_pos_;
								--new_data_end_begin;

								real_data_head->new_data_pos_ += move_data_length;
								real_data_head->data_array_size_++;
							}
							num++;
						}
					}
					else//文件中没有该标签点
					{
						datafile_mgr_->AllocBlock(data_id, file_set, t, index, data_head, data_end, real_data_head, real_data_end);
						void* real_new_data_pos = file_set->daa_.get_absolute_pos((void*)(real_data_head->new_data_pos_));
						memcpy(real_new_data_pos, data_iter->data().c_str(), data_iter->data().length());
						DataEndArray *new_data_end_begin = real_data_end - real_data_head->data_array_size_;
						new_data_end_begin->datetime_ = t;
						new_data_end_begin->data_pos_ = real_data_head->new_data_pos_;
						real_data_head->new_data_pos_ += data_iter->data().length();
						real_data_head->data_array_size_++;
						num++;
					}
				}
				else//超出时间范围
				{
					if (begin_t == data_iter->time_stamp())
						++data_iter;//避免死循环
					break;
				}
			}
			--data_iter;
		}
	}
	return ret;
}

// int DataMgr::GetDataInfo(int data_id, time_t t, void* &data, int &data_length)
// {
// 	int ret = MYDATA_OK;
// 	pair<time_t, DataFileInfoInner *> datafile = datafile_mgr_->GetFile(t);
// 	if (datafile.first == -1)
// 		return MYDATA_DATA_NOT_EXIST;
// 
// 	FileSet *file_set = datafile.second->file_set_;
// 	DataIndex index;
// 	if (file_set->index_mgr_.GetSkipList(data_id, &index))
// 		return MYDATA_POINT_NOT_EXIST;
// 	DataIndex::Iterator iter(&index);
// 	iter.Seek(t);
// 	if (iter.Valid())
// 	{
// 		DataHead* data_head = NULL;
// 		DataEndArray *data_end = NULL;
// 		DataHead *real_data_head = NULL;
// 		DataEndArray *real_data_end = NULL;
// 
// 		data_head = (DataHead*)iter.value();
// 		data_end = GetDataEnd(data_head);
// 		real_data_head = (DataHead*)file_set->daa_.get_absolute_pos(data_head);
// 		real_data_end = file_set->daa_.get_absolute_pos(data_end);
// 		DataEndArray *data_end_begin = real_data_end - real_data_head->data_array_size_ + 1;
// 		DataEndArray end_value;
// 		end_value.datetime_ = t;
// 
// 		DataEndArray* find_value = std::lower_bound(data_end_begin, real_data_end + 1, end_value, CompareDataEnd);
// 		if (find_value == real_data_end + 1 || find_value->datetime_ != t)
// 			return MYDATA_DATA_NOT_EXIST;
// 		void* found_data = file_set->daa_.get_absolute_pos((void*)(find_value->data_pos_));
// 		//计算数据的长度
// 		if (real_data_end - find_value == real_data_head->data_array_size_ - 1)
// 		{//数组的最后一个
// 			data_length = real_data_head->new_data_pos_ - find_value->data_pos_;
// 		}
// 		else
// 		{
// 			data_length = (find_value - 1)->data_pos_ - find_value->data_pos_;
// 		}
// 		data = found_data;
// 	}
// 	else
// 	{
// 		ret = MYDATA_DATA_NOT_EXIST;
// 	}
// 	return ret;
// }

int DataMgr::ReadNewData(int data_id, time_t &t, void* data, int &data_length)
{
	int ret = MYDATA_DATA_NOT_EXIST;
	t = -1;
	RLOCKGUARD(_rwlock);
	auto &mgr = datafile_mgr_->GetMgr();
	SkipListKVDisk<time_t, DataFileInfoInner, CommonLowThan<time_t>>::Iterator iter(&mgr);
	for (iter.SeekToFirst(); iter.Valid() && iter.key() != -1; iter.Next())
	{
		FileSet *file_set = iter.value().file_set_;
		DataIndex index;
		RLOCKGUARD(file_set->rwlock_);
		if (file_set->index_mgr_.GetSkipList(data_id, &index))
			continue;
		DataIndex::Iterator data_iter(&index);
		data_iter.SeekToFirst();
		if (data_iter.Valid())
		{
			DataHead* data_head = NULL;
			DataEndArray *data_end = NULL;
			DataHead *real_data_head = NULL;
			DataEndArray *real_data_end = NULL;

			data_head = (DataHead*)data_iter.value();
			data_end = GetDataEnd(data_head);
			real_data_head = (DataHead*)file_set->daa_.get_absolute_pos(data_head);
			real_data_end = file_set->daa_.get_absolute_pos(data_end);
			DataEndArray *data_end_begin = real_data_end - real_data_head->data_array_size_ + 1;
			t = data_end_begin->datetime_;
			void* fount_data = file_set->daa_.get_absolute_pos((void*)(data_end_begin->data_pos_));
			data_length = real_data_head->new_data_pos_ - data_end_begin->data_pos_;
			memcpy(data, fount_data, data_length);
			ret = MYDATA_OK;
		}
		else
		{
			ret = MYDATA_DATA_NOT_EXIST;
		}
		break;
	}
	
	return ret;
}

int DataMgr::ReadDatasNum(int data_id, time_t time_begin, time_t time_end, int &num)
{
	int ret = MYDATA_OK;
	num = 0;
	RLOCKGUARD(_rwlock);
	auto &mgr = datafile_mgr_->GetMgr();
	SkipListKVDisk<time_t, DataFileInfoInner, CommonLowThan<time_t>>::Iterator iter(&mgr);
	for (iter.Seek(time_end); iter.Valid(); iter.Next())
	{
		DataFileInfoInner &fileinfo = iter.value();
		if (fileinfo.end_time_ < time_begin)//不在范围内，或是空数据文件
			break;
		FileSet *file_set = fileinfo.file_set_;
		RLOCKGUARD(file_set->rwlock_);
		DataIndex index;
		if (file_set->index_mgr_.GetSkipList(data_id, &index))
			continue;
		DataIndex::Iterator data_iter(&index);
		for (data_iter.Seek(time_end); data_iter.Valid(); data_iter.Next())
		{
			DataHead* data_head = NULL;
			DataEndArray *data_end = NULL;
			DataHead *real_data_head = NULL;
			DataEndArray *real_data_end = NULL;

			data_head = (DataHead*)data_iter.value();
			data_end = GetDataEnd(data_head);
			real_data_head = (DataHead*)file_set->daa_.get_absolute_pos(data_head);
			real_data_end = file_set->daa_.get_absolute_pos(data_end);
			DataEndArray *data_end_begin = real_data_end - real_data_head->data_array_size_ + 1;
			for (DataEndArray* data_element = data_end_begin; 
				data_element <= real_data_end; 
				data_element++)
			{
				if (data_element->datetime_ >= time_begin && data_element->datetime_ <=time_end)
					num++;
			}
		}
	}
	if (num == 0)
		ret = MYDATA_DATA_NOT_EXIST;
	return ret;
}

int DataMgr::ReadDatas(int data_id, time_t time_begin, time_t time_end, Datas::ReadDatasReply &reply)
{
	int ret = MYDATA_OK;
	int num = 0;
	int data_length = 0;
	auto datas = reply.mutable_data_infos();
	datas->Reserve(DATAS_QUERY_BUFFER);
	RLOCKGUARD(_rwlock);
	auto &mgr = datafile_mgr_->GetMgr();
	SkipListKVDisk<time_t, DataFileInfoInner, CommonLowThan<time_t>>::Iterator iter(&mgr);
	for (iter.Seek(time_end); iter.Valid(); iter.Next())
	{
		DataFileInfoInner &fileinfo = iter.value();
		if (fileinfo.end_time_ < time_begin)//不在范围内，或是空数据文件
			break;
		FileSet *file_set = fileinfo.file_set_;
		RLOCKGUARD(file_set->rwlock_);
		DataIndex index;
		if (file_set->index_mgr_.GetSkipList(data_id, &index))
			continue;
		DataIndex::Iterator data_iter(&index);
		for (data_iter.Seek(time_end); data_iter.Valid(); data_iter.Next())
		{
			DataHead* data_head = NULL;
			DataEndArray *data_end = NULL;
			DataHead *real_data_head = NULL;
			DataEndArray *real_data_end = NULL;

			data_head = (DataHead*)data_iter.value();
			data_end = GetDataEnd(data_head);
			real_data_head = (DataHead*)file_set->daa_.get_absolute_pos(data_head);
			real_data_end = file_set->daa_.get_absolute_pos(data_end);
			DataEndArray *data_end_begin = real_data_end - real_data_head->data_array_size_ + 1;
			for (DataEndArray* data_element = data_end_begin; 
				data_element <= real_data_end; 
				data_element++)
			{
				if (data_element->datetime_ >= time_begin && data_element->datetime_ <=time_end)
				{
					void *real_data = file_set->daa_.get_absolute_pos((void*)data_element->data_pos_);
					if (real_data_end - data_element == real_data_head->data_array_size_ - 1)
					{//数组的最后一个
						data_length = real_data_head->new_data_pos_ - data_element->data_pos_;
					}
					else
					{
						data_length = (data_element - 1)->data_pos_ - data_element->data_pos_;
					}
					auto data = datas->Add();
					data->set_data(real_data, data_length);
					data->set_time_stamp(data_element->datetime_);
					if (++num >= DATAS_QUERY_BUFFER)
					{
						return MYDATA_OK;
					}
				}
			}
		}
	}
	if (num < DATAS_QUERY_BUFFER)
		return MYDATA_DATA_QUERY_END;
	return ret;
}

int DataMgr::RemoveDatas(Datas::RemoveDatasQuery &query)
{
	int id = query.id();
	auto timestamps = query.time_stamps();
	WLOCKGUARD(_rwlock);
	auto &mgr = datafile_mgr_->GetMgr();
	SkipListKVDisk<time_t, DataFileInfoInner, CommonLowThan<time_t>>::Iterator iter(&mgr);
	for (auto time_iter = timestamps.begin(); time_iter != timestamps.end(); ++time_iter)
	{
		iter.Seek(*time_iter);
		if (iter.Valid())
		{
			DataFileInfoInner &fileinfo = iter.value();
			if (*time_iter > fileinfo.end_time_ || *time_iter < fileinfo.start_time_)
			{//没有合适的数据文件
				continue;
			}
			FileSet *file_set = fileinfo.file_set_;
			WLOCKGUARD(file_set->rwlock_);
			DataIndex index;
			if (file_set->index_mgr_.GetSkipList(id, &index))
			{//合适的数据文件里面没有指定的标签点id
				continue;
			}
			DataIndex::Iterator index_iter(&index);
			time_t t = *time_iter;
			for (;time_iter != timestamps.end();)
			{
				index_iter.Seek(*time_iter);
				if (index_iter.Valid() && *time_iter <= fileinfo.end_time_)
				{
					DataHead* data_head = NULL;
					DataEndArray *data_end = NULL;
					DataHead *real_data_head = NULL;
					DataEndArray *real_data_end = NULL;

					data_head = (DataHead*)index_iter.value();
					data_end = GetDataEnd(data_head);
					real_data_head = (DataHead*)file_set->daa_.get_absolute_pos(data_head);
					real_data_end = file_set->daa_.get_absolute_pos(data_end);
					DataEndArray *data_end_begin = real_data_end - real_data_head->data_array_size_ + 1;
					DataEndArray end_value;
					end_value.datetime_ = *time_iter;
					DataEndArray* find_value = std::lower_bound(data_end_begin, real_data_end + 1, end_value, CompareDataEnd);
					if (find_value->datetime_ != *time_iter)
					{
						++time_iter;
						continue;
					}
					DataEndArray* next_value = find_value-1;
					while (++time_iter != timestamps.end() && next_value != data_end_begin-1)
					{
						if (*time_iter != next_value->datetime_)
							break;
						--next_value;
					}
					if (find_value == real_data_end && next_value == data_end_begin-1)
					{//整块删除
						real_data_head->dirty_ = 0;
						index.Erase(index_iter.key());
						file_set->skd_.Push(data_head);
					}
					else
					{//部分删除，删除的是第一个数会导致索引的key与块的开始时间不一致
						for (; next_value >= data_end_begin; next_value--, find_value--)
							*find_value = *next_value;
						real_data_head->data_array_size_ -= find_value-next_value;
					}
				}
				else//没有合适的索引
				{
					if (t == *time_iter)
						++time_iter;//避免死循环
					break;
				}
			}
			--time_iter;
		}
	}
	return MYDATA_OK;
}

int DataMgr::RemovePoint(int data_id)
{
	WLOCKGUARD(_rwlock);
	auto &mgr = datafile_mgr_->GetMgr();
	SkipListKVDisk<time_t, DataFileInfoInner, CommonLowThan<time_t>>::Iterator iter(&mgr);
	for (iter.SeekToFirst(); iter.Valid(); iter.Next())
	{
		DataFileInfoInner &fileinfo = iter.value();
		FileSet *file_set = fileinfo.file_set_;
		WLOCKGUARD(file_set->rwlock_);
		DataIndex index;
		if (file_set->index_mgr_.GetSkipList(data_id, &index))
			continue;
		DataHead *data_head = NULL;
		DataHead *real_data_head = NULL;
		DataIndex::Iterator data_iter(&index);
		for (data_iter.SeekToFirst(); data_iter.Valid(); data_iter.Next())
		{
			data_head = (DataHead*)data_iter.value();
			real_data_head = (DataHead*)file_set->daa_.get_absolute_pos(data_head);
			real_data_head->dirty_ = 0;
			file_set->skd_.Push(data_head);
		}
		file_set->index_mgr_.EraseSkipList(data_id);
	}
	return MYDATA_OK;
}

void DataMgr::StopRunning()
{
	_run = false;
}
bool DataMgr::IsRunning()
{
	return _run;
}
int DataMgr::CreateFile(const char* filename, long long file_size, time_t start_time, time_t end_time)
{
	return datafile_mgr_->CreateFile(filename, file_size, start_time, end_time);
}

int DataMgr::DeleteFile(const char* filename, time_t start_time)
{
	return datafile_mgr_->DeleteFile(filename, start_time);
}

int DataMgr::AddFile(const char* filename)
{
	return datafile_mgr_->AddFile(filename);
}

int DataMgr::GetFilesNum()
{
	return datafile_mgr_->GetFilesNum();
}

void DataMgr::GetFileInfos(Datas::DataFileInfosReply &fileinfos)
{
	return datafile_mgr_->GetFileInfos(fileinfos);
}

int DataMgr::ReBuildIndex(const char* filename, time_t start_time)
{
	return datafile_mgr_->ReBuildIndex(filename, start_time);
}

int DataMgr::ReBuildDataFileMgr()
{
	WLOCKGUARD(_rwlock);
	return datafile_mgr_->GetMgr().ReBuild();
}