#include "stdafx.h"
#include "mymmapw.h"


mymmap::mymmap(void)
{
	handle_ = INVALID_HANDLE_VALUE;
	file_mapping_ = INVALID_HANDLE_VALUE;
	base_addr_ = (void*)-1;
	length_ = 0;
	close_handle_ = false;
	memset(filename_, 0 ,sizeof(filename_));
}


mymmap::~mymmap(void)
{
	close();
}

int mymmap::open (const char *filename,	int flags, int perms)
{
	close_handle();
	handle_ = CreateFile(filename, flags, perms, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (handle_ == INVALID_HANDLE_VALUE)
		return -1;

	strcpy_s(filename_, filename);
	close_handle_ = true;
	return 0;
}
BOOL mymmap::get_filesize(long long *filesize)
{
	LARGE_INTEGER fs;
	if (!GetFileSizeEx(handle_, &fs))
	{
		return FALSE;
	}
	*filesize = fs.QuadPart;
	return TRUE;
}
int mymmap::map(HANDLE handle, size_t &length, int prot, long long offset)
{
	int unmap_result = unmap();
	if (unmap_result)
		return unmap_result;
	handle_ = handle;
	LARGE_INTEGER filesize;
	if (!GetFileSizeEx(handle, &filesize))
	{
		return -1;
	}
	if (length == 0)
	{
		if (filesize.QuadPart == 0)
		{
			SYSTEM_INFO sinfo;
			GetSystemInfo(&sinfo);
			length = sinfo.dwPageSize;
		}
		else
		{
			length = filesize.QuadPart;
		}
	}
	long long request_file_length = length + offset;
	if (request_file_length > filesize.QuadPart)
	{
		long long null_pos = request_file_length - 1;
		if (write_file(handle_, "", 1, null_pos) == -1)
			return -1;
	}
	LARGE_INTEGER offset_t;
	offset_t.QuadPart = offset;
	file_mapping_ = CreateFileMapping(handle_, NULL, prot, 0,0, NULL);
	if (file_mapping_ == NULL)
	{
		return -1;
	}
	
	base_addr_ = MapViewOfFile(file_mapping_, prot & PAGE_READWRITE ? FILE_MAP_WRITE : FILE_MAP_READ, 
		offset_t.HighPart, offset_t.LowPart, length);
	length_ = length;
	if (base_addr_)
		return 0 ;
	else
	{
		base_addr_ = (void*)-1;
		return -1;
	}
}
int mymmap::map(const char *filename, size_t &length, int prot,	long long offset)
{
	if (open(filename) == -1)
		return -1;
	return map(handle_, length, prot, offset);
}
int mymmap::map(size_t &length, int prot, long long offset)
{
	return map(handle_, length, prot, offset);
}
int mymmap::close_filemapping_handle()
{
	int result = 0;
	if (file_mapping_ != handle_
		&& this->file_mapping_ != INVALID_HANDLE_VALUE)
	{
		result = CloseHandle(file_mapping_) ? 0 : -1;
		this->file_mapping_ = INVALID_HANDLE_VALUE;
	}
	return result;
}
int mymmap::close_handle()
{
	int result = 0;
	if (close_handle_)
	{
		close_handle_ = false;
		result = CloseHandle(this->handle_) ? 0 : -1;
		this->handle_ = INVALID_HANDLE_VALUE;
	}
	return result;
}
void* mymmap::addr()
{
	return base_addr_;
}
const char* mymmap::filename()
{
	return filename_;
}
size_t mymmap::size()
{
	return length_;
}
BOOL mymmap::sync(void* addr, size_t len)
{
	return FlushViewOfFile(addr, len);
}
BOOL mymmap::sync(size_t len)
{
	return FlushViewOfFile(base_addr_, len);
}
BOOL mymmap::sync()
{
	return FlushViewOfFile(base_addr_, length_);
}
int mymmap::unmap()
{
	close_filemapping_handle ();

	if (base_addr_ != (void*)-1)
	{
		int result = UnmapViewOfFile(base_addr_) != 0 ? 0 : 1;
		base_addr_ = (void*)-1;
		return result;
	}
	else
		return 0;
}
int mymmap::close()
{
	unmap ();

	return close_handle ();
}
int mymmap::write_file(HANDLE handle, const void *buf, size_t nbytes, long long offset)
{
	LARGE_INTEGER original_high_position;
	LARGE_INTEGER move_position;
	move_position.HighPart = move_position.LowPart = 0;
	if (!SetFilePointerEx(handle, move_position, &original_high_position, FILE_CURRENT))
		return -1;
	move_position.QuadPart = offset;
	if (!SetFilePointerEx(handle, move_position, NULL, FILE_BEGIN))
		return -1;
	DWORD bytes_written;
	if (!WriteFile(handle, buf, nbytes, &bytes_written, NULL))
		return -1;
	if (!SetFilePointerEx(handle, original_high_position, NULL, FILE_BEGIN))
		return -1;
	return bytes_written;
}