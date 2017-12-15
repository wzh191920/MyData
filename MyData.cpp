// MyData.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "Service.h"
#include <iostream>
#include <algorithm>
#include "ace/Init_ACE.h"
#include "SqlHandler.h"
#include "DataMgr.h"
#include "PlugIn.h"
#include "threadpoolw.h"
#include "MyDataConfig.h"
#include "MyDataCore.h"
#include "LogFilew.h"
using namespace std;

threadpool _tp;
SqlHandler _sql_handler;
PlugIn _plugin;
DataMgr _dm;
MyDataConfig _config;
unsigned int __stdcall Jobs(void*);
unsigned int __stdcall MyDataMain(void*)
{
	ACE::init();
	_config.Init();
	CLogFile::Instance()->Open("MyData.log", _config._log_path, false);
	CLogFile::Instance()->SetPriority(CLogFile::debug);
	LOG_INFO_WRITE("服务启动");
	proto_handler::init();
	ACE_INET_Addr listenaddr(_config._port); 
	ACE_Asynch_Acceptor<MyDataServiceHandler> acceptor;
	int ret = acceptor.open(listenaddr, 0, true);
	if (ret)
	{
		LOG_ERROR_WRITE("端口绑定失败");
		return ret;
	}
	ret = _tp.init(_config._thread_num);
	if (ret)
	{
		LOG_ERROR_WRITE("线程池初始化失败");
		return ret;
	}
	ret = _sql_handler.Init();
	if (ret)
	{
		LOG_ERROR_WRITE("sql初始化失败");
		return ret;
	}
	ret = _dm.Init(CLogFile::Instance(), &_config);
	if (ret)
	{
		LOG_ERROR_WRITE("数据初始化失败");
		return ret;
	}
	HANDLE thandle = (HANDLE)_beginthreadex( NULL, 0, Jobs, 0, 0, NULL);
	if (thandle == 0)
		return -1;
	else
		CloseHandle(thandle);
	_plugin.Init();
	ret = ACE_Proactor::instance()->proactor_run_event_loop();
	LOG_INFO_WRITE("服务停止");
	ACE::fini();
	return 0;
}

void EndEventLoop()
{
	ACE_Proactor::instance()->proactor_end_event_loop();
}


unsigned int __stdcall Jobs(void* data)
{
	JobDesc job_desc;
	job_desc.desc.reserve(JOB_QUERY_BUFFER);
	job_desc.desc2.reserve(JOB_QUERY_BUFFER);
	int ret = 0;
	time_t rebuild_time = time(0);
	int rebuild_filenum = _dm.GetFilesNum();
	bool has_job = false;
	while (DataMgr::_run)
	{
		//处理删点任务
		ret = _sql_handler.GetJobs(SqlHandler::delete_point, job_desc, JOB_QUERY_BUFFER);
		if (ret == 0 && !job_desc.desc2.empty())
		{
			for (vector<int>::iterator iter = job_desc.desc2.begin(); iter != job_desc.desc2.end(); ++iter)
			{
				_dm.RemovePoint(*iter);
			}
			_sql_handler.ClearJobs(SqlHandler::delete_point, job_desc);
			job_desc.desc2.clear();
			has_job = true;
		}
		//处理删表任务
		ret = _sql_handler.GetJobs(SqlHandler::delete_table, job_desc, 1);
		if (ret == 0 && !job_desc.desc2.empty())
		{
			int table_id = job_desc.desc2[0];
			job_desc.desc2.clear();
			ret = _sql_handler.GetDeletePointsByTableID(table_id, job_desc.desc2);
			if (ret == 0 && !job_desc.desc2.empty())
			{
				for (vector<int>::iterator iter = job_desc.desc2.begin(); iter != job_desc.desc2.end(); ++iter)
				{
					_dm.RemovePoint(*iter);
				}
				_sql_handler.ClearDeletePoints(job_desc.desc2);
			}
			if (job_desc.desc2.empty())
			{
				job_desc.desc2.push_back(table_id);
				_sql_handler.ClearJobs(SqlHandler::delete_table, job_desc);
			}
			job_desc.desc2.clear();
			has_job = true;
		}
		if (has_job)
		{
			has_job = false;
			Sleep(100);
			continue;
		}
		if (rebuild_time + 24*3600*180 < time(0))//每半年文件数量有变化时重建一次
		{
			if (rebuild_filenum != _dm.GetFilesNum())
			{
				ret = _dm.ReBuildDataFileMgr();
				if (ret)
					LOG_ERROR_WRITE("重建管理文件索引失败, %d", ret);
				rebuild_filenum = _dm.GetFilesNum();
			}
			rebuild_time = time(0);
		}
		Sleep(10000);
	}
	return 0;
}