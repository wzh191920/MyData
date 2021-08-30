#pragma once
#include "windows.h"
class mymmap
{
public:
	mymmap(void);
	~mymmap(void);
	int open (const char *filename,
		int flags = GENERIC_READ | GENERIC_WRITE,
		int perms = FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE);
	int map(HANDLE handle,
		size_t &length,
		int prot = PAGE_READWRITE,
		long long offset = 0);
	int map(const char *filename,
		size_t &length,
		int prot = PAGE_READWRITE,
		long long offset = 0);
	int map (size_t &length,
		int prot = PAGE_READWRITE,
		long long offset = 0);
	void* addr();
	const char* filename();
	size_t size();
	BOOL get_filesize(long long *filesize);
	BOOL sync(void* addr, size_t len);
	BOOL sync(size_t len);
	BOOL sync();
	int unmap();
	int close_handle();
	int close_filemapping_handle();
	int close();
protected:
	int write_file(HANDLE handle, const void *buf, size_t nbytes, long long offset);
	HANDLE handle_;
	HANDLE file_mapping_;
	void *base_addr_;
	size_t length_;
	char filename_[MAX_PATH];
	bool close_handle_;
};

