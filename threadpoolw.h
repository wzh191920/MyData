#pragma once
#include "windows.h"
#include <queue>
using std::queue;

struct thread_task
{
	void* data;
	void* (*callback_fun)(void*);
};

class task_queue
{
public:
	int init();
	int close();
	void puttask(thread_task& task);
	thread_task gettask();
private:
	CRITICAL_SECTION cs_;
	HANDLE event_;
	queue<thread_task> tasks_;
};

class threadpool
{
public:
	threadpool();
	int init(int thread_num);
	int close();
	void puttask(thread_task& task);
	thread_task gettask();
	virtual ~threadpool();
	bool run_;
private:
	static unsigned int __stdcall thread_function(void* data);
	task_queue task_queue_;
	CRITICAL_SECTION cs_;
	HANDLE *thread_handles_;
	int thread_num_;
};