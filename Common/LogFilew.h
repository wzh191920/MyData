#ifndef _ATA_LOGFILE_
#define _ATA_LOGFILE_

#include <stdio.h>
#include <set>
#include <string>
#include "Windows.h"
using std::string;
using std::set;
class CLogFile
{
public:
	enum {debug, info, warning, error};
	~CLogFile();
	static CLogFile* Instance();
	static void Instance(CLogFile* instance);
	void SetPriority(int priority);
	void SetLogCount(int count);
	int Open(char* strFile, char* dir = ".", bool async = true, bool bAppend = true, long lTruncate = 5242880);
	void WriteEx(int priority, int line, char* logbuf);
	void Write(LPCTSTR pszFormat, ...);

	class AutoDelete
	{
	public:
		AutoDelete():obj(NULL){}
		~AutoDelete()
		{
			if (obj)
			{
				delete obj;
				obj = NULL;
			}
		}
		CLogFile* obj;
	};
private:
	CLogFile();
	void CheckFilesize(SYSTEMTIME &time);
	char*		m_szPriority[4];
	char		m_szFile[MAX_PATH + 1];
	FILE*		m_pLogFile;
	char		m_Dir[MAX_PATH*2];
	long		m_lTruncate;
	set<string> m_FileList;
	CRITICAL_SECTION	m_cs;
	int			m_fCount;
	int			m_Priority;
	HANDLE      m_event;
	HANDLE		m_thead;
	bool 		m_sync;
	static AutoDelete m_obj;
	static CLogFile*   m_Instance;
	static unsigned __stdcall FlashFunc( void* pArguments );
};

#define LOG_ERROR_WRITE(FORMAT,...) do{\
	char	szLog[512];\
	sprintf( szLog, FORMAT, ## __VA_ARGS__ );\
	CLogFile::Instance()->WriteEx(CLogFile::error, __LINE__, szLog);}while(0)

#define LOG_WARNING_WRITE(FORMAT,...) do{\
	char	szLog[512];\
	sprintf( szLog, FORMAT, ## __VA_ARGS__ );\
	CLogFile::Instance()->WriteEx(CLogFile::warning, __LINE__, szLog);}while(0)

#define LOG_INFO_WRITE(FORMAT,...) do{\
	char	szLog[512];\
	sprintf( szLog, FORMAT, ## __VA_ARGS__ );\
	CLogFile::Instance()->WriteEx(CLogFile::info, __LINE__, szLog);}while(0)

#define LOG_DEBUG_WRITE(FORMAT,...) do{\
	char	szLog[512];\
	sprintf( szLog, FORMAT, ## __VA_ARGS__ );\
	CLogFile::Instance()->WriteEx(CLogFile::debug, __LINE__, szLog);}while(0)
#endif //_ATA_LOGFILE_
