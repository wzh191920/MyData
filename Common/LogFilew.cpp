#include "stdafx.h"
#include <process.h>
#include "LogFilew.h"

CLogFile* CLogFile::m_Instance = NULL;
CLogFile::AutoDelete CLogFile::m_obj;
unsigned __stdcall CLogFile::FlashFunc( void* pArguments )
{
	CLogFile* cf = (CLogFile*)pArguments;
	do 
	{
		DWORD ret = WaitForSingleObject(cf->m_event, 30000);
		if (ret != WAIT_TIMEOUT)
			break;
		EnterCriticalSection(&cf->m_cs);
		fflush(cf->m_pLogFile);
		SYSTEMTIME	time;
		::GetLocalTime(&time);
		cf->CheckFilesize(time);
		LeaveCriticalSection(&cf->m_cs);
	} while (1);
	return 0;
}

CLogFile::CLogFile():m_fCount(5)
{
	m_pLogFile = NULL;
	m_Priority = info;
	m_szPriority[debug] = "debug";
	m_szPriority[info] = "info";
	m_szPriority[warning] = "warning";
	m_szPriority[error] = "error";
	m_event = NULL;
	m_thead = NULL;
	m_sync = true;
	InitializeCriticalSection(&m_cs);
}

CLogFile::~CLogFile()
{
	if (m_event)
	{
		SetEvent(m_event);
		DWORD ret = WaitForSingleObject(m_thead, 2000);
		if (ret == WAIT_TIMEOUT)
		{
			SuspendThread(m_thead);
		}
		CloseHandle(m_thead);
		CloseHandle(m_event);
	}
	
	if (m_pLogFile)
		fclose(m_pLogFile);
	DeleteCriticalSection(&m_cs);
	m_Instance = NULL;
	m_pLogFile = NULL;
}
CLogFile* CLogFile::Instance()
{
	if (m_Instance == NULL)
	{
		m_Instance = new CLogFile();
		m_obj.obj = m_Instance;
	}
	return m_Instance;
}
void CLogFile::Instance(CLogFile* instance)
{
	m_Instance = instance;
}
void CLogFile::SetPriority(int priority)
{
	m_Priority = priority;
}
void CLogFile::SetLogCount(int count)
{
	m_fCount = count;
}
int CLogFile::Open(char* strFile, char* dir, bool async, bool bAppend, long lTruncate)
{
	int ret = -1;
	if (strlen(strFile)>3)
	{
		strcpy(m_szFile, strFile);
		strcpy(m_Dir, dir);
		if (m_pLogFile)
			return ret;
		char fullFile[MAX_PATH*2];
		sprintf(fullFile, "%s/%s", dir, strFile);
		m_pLogFile = fopen(fullFile, bAppend ? "a" : "w");
		m_lTruncate = lTruncate;
		if (m_pLogFile != NULL)
			ret = 0;

		WIN32_FIND_DATA FindFileData;
		HANDLE hFind;
		char t[MAX_PATH];
		strcpy(t, dir);
		strcat(t, "*");
		hFind = FindFirstFile(t, &FindFileData);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			size_t allFnLen = strlen(m_szFile);
			char *comaPos = strrchr(m_szFile, '.');
			if (comaPos == NULL)
			{
				return -1;
			}
			size_t fnLen = comaPos-m_szFile;
			do
			{
				if (strncmp(m_szFile, FindFileData.cFileName, fnLen)==0
					&& !(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					&& strlen(FindFileData.cFileName)>allFnLen)
				{
					m_FileList.insert(FindFileData.cFileName);
				}
			}
			while (FindNextFile(hFind, &FindFileData) != 0);
			FindClose(hFind);
		}
		m_sync = !async;
		if (async)
		{
			m_event = CreateEvent(NULL, FALSE, FALSE, NULL);
			if (m_event == NULL)
			{
				return -1;
			}
			unsigned threadID;
			m_thead = (HANDLE)_beginthreadex(NULL, 0, FlashFunc, this, 0, &threadID);
			if (m_thead == 0)
			{
				return -1;
			}
		}
	}
	return ret;
}
void CLogFile::WriteEx(int priority, int line, char* logbuf)
{
	if (m_Priority > priority)
		return;
	if (!m_pLogFile)
		return;
	EnterCriticalSection(&m_cs);			

	//Get current time
	SYSTEMTIME	time;
	::GetLocalTime(&time);
	char	szLine[1024];

	sprintf(szLine, "%04d-%02d-%02d %02d:%02d:%02d:%03d L%d, %s, %s\n", 
		time.wYear, time.wMonth, time.wDay, 
		time.wHour, time.wMinute, time.wSecond, time.wMilliseconds,
		line, m_szPriority[priority], logbuf);

	fputs(szLine, m_pLogFile);
	if (m_sync)
	{
		fflush(m_pLogFile);
		CheckFilesize(time);
	}	
	LeaveCriticalSection(&m_cs);
}
void CLogFile::Write(LPCTSTR pszFormat, ...)
{
	if (!m_pLogFile)
		return;

	EnterCriticalSection(&m_cs);
	char	szBuf[512];
	va_list argList;
	va_start( argList, pszFormat );
	vsprintf( szBuf, pszFormat, argList );
	va_end( argList );		

	SYSTEMTIME	time;
	::GetLocalTime(&time);
	char	szLine[1024];

	sprintf(szLine, "%04d-%02d-%02d %02d:%02d:%02d:%03d, %s\n", 
		time.wYear, time.wMonth, time.wDay, 
		time.wHour, time.wMinute, time.wSecond, time.wMilliseconds,
		szBuf);

	fputs(szLine, m_pLogFile);
	if (m_sync)
	{
		fflush(m_pLogFile);
		CheckFilesize(time);
	}	

	LeaveCriticalSection(&m_cs);
}
void CLogFile::CheckFilesize(SYSTEMTIME &time)
{
	long lLength = ftell(m_pLogFile);
	if (lLength > m_lTruncate)
	{
		char szBuf[MAX_PATH];
		fclose(m_pLogFile);
		sprintf(szBuf, "%s%s", m_Dir, m_szFile);
		char *comaPos = strrchr(m_szFile, '.');
		if (comaPos != NULL)
		{
			size_t fnLen = comaPos-m_szFile;
			char fn[MAX_PATH]={0};
			char ext[32]={0};
			strncpy(fn, m_szFile, fnLen);
			strcpy(ext, comaPos);
			char szLine[512];
			sprintf(szLine, "%s%04d%02d%02d_%02d%02d%02d%s", fn,
				time.wYear, time.wMonth, time.wDay, 
				time.wHour, time.wMinute, time.wSecond, ext);
			m_FileList.insert(szLine);
			sprintf(fn, "%s%s", m_Dir, szLine);
			MoveFile(szBuf, fn);

			while (m_FileList.size() > m_fCount)
			{
				sprintf(szLine, "%s%s", m_Dir, m_FileList.begin()->c_str());
				DeleteFile(szLine);
				m_FileList.erase(m_FileList.begin());
			}
		}
		m_pLogFile = fopen(szBuf, "w");
	}
}