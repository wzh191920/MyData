#include "StdAfx.h"
#include "process.h"
#include <exception>
#include "threadpoolw.h"

class CloseException : public std::exception
{
public:
	virtual const char* what()
	{
		return "closed";
	}
}_close_exception;
void* CloseFunction(void*)
{
	throw _close_exception;
	return NULL;
}

int task_queue::init()
{
	InitializeCriticalSection(&cs_);
	event_ = CreateEvent(NULL, FALSE, FALSE, NULL); 
	return 0;
}
int task_queue::close()
{
	CloseHandle(event_);
	DeleteCriticalSection(&cs_);
	return 0;
}
void task_queue::puttask(thread_task& task)
{
	EnterCriticalSection(&cs_);
	tasks_.push(task);
	SetEvent(event_);
	LeaveCriticalSection(&cs_);
}
thread_task task_queue::gettask()
{
	DWORD ret;

	EnterCriticalSection(&cs_);
	while (tasks_.empty())
	{
		LeaveCriticalSection(&cs_);
		ret = WaitForSingleObject(event_, INFINITE);
		if (ret != WAIT_OBJECT_0)
		{
			thread_task ptask;
			ptask.callback_fun = NULL;
			return ptask;
		}
		EnterCriticalSection(&cs_);
	}
	thread_task ptask = tasks_.front();
	tasks_.pop();
	LeaveCriticalSection(&cs_);
	return ptask;
}


threadpool::threadpool()
{
	run_ = false;
}

threadpool::~threadpool()
{
	close();
}

int threadpool::init(int thread_num)
{
	run_ = true;
	thread_num_ = thread_num;
	thread_handles_ = new(std::nothrow) HANDLE[thread_num];
	if (thread_handles_ == NULL)
		return -1;
	memset(thread_handles_, 0, sizeof(*thread_handles_)*thread_num);
	task_queue_.init();
	InitializeCriticalSection(&cs_);
	for (int i=0; i<thread_num_; i++)
	{
		thread_handles_[i] = (HANDLE)_beginthreadex( NULL, 0, thread_function, this, 0, NULL);
		if (thread_handles_[i] == 0)
			return -1;
	}

	return 0;
}
int threadpool::close()
{
	if (run_)
	{
		run_ = false;
	}
	thread_task task;
	task.callback_fun = CloseFunction;
	for (int i=0; i<thread_num_; i++)
		task_queue_.puttask(task);
	for (int i=0; i<thread_num_; i++)
	{
		WaitForSingleObject(thread_handles_[i], 2000);
		CloseHandle(thread_handles_[i]);
	}
	task_queue_.close();
	DeleteCriticalSection(&cs_);
	return 0;
}
void threadpool::puttask(thread_task& task)
{
	task_queue_.puttask(task);
}
thread_task threadpool::gettask()
{
	return task_queue_.gettask();
}
unsigned int threadpool::thread_function(void* data)
{
	threadpool *This = (threadpool*)data;
	try
	{
		while (This->run_)
		{
			EnterCriticalSection(&This->cs_);
			thread_task task = This->gettask();
			LeaveCriticalSection(&This->cs_);
			(task.callback_fun)(task.data);
		}

	}
	catch (CloseException &e)
	{
	}	
	_endthreadex( 0 );
	return 0;
}
