#include "stdafx.h"
#include <windows.h>
#include <process.h>
#include <tchar.h>
#include <strsafe.h>
#include "DataMgr.h"
#include "MyData.h"

extern DataMgr _dm;
#define SVCNAME TEXT("MyData Server")
#define SVC_ERROR                        ((DWORD)0xC0020001L)

SERVICE_STATUS          gSvcStatus; 
SERVICE_STATUS_HANDLE   gSvcStatusHandle; 
HANDLE                  ghSvcStopEvent = NULL;

VOID SvcInstall(void);
VOID WINAPI SvcCtrlHandler( DWORD ); 
VOID WINAPI SvcMain( DWORD, LPTSTR * ); 

VOID ReportSvcStatus( DWORD, DWORD, DWORD );
VOID SvcInit( DWORD, LPTSTR * ); 
VOID SvcReportEvent( LPTSTR );

VOID SvcDelete(void);

SC_HANDLE schSCManager;
SC_HANDLE schService;

VOID __stdcall DoStartSvc(char*);
VOID __stdcall DoStopSvc(char*);
BOOL __stdcall StopDependentServices();

void __cdecl _tmain(int argc, TCHAR *argv[]) 
{ 
	MyDataMain(NULL);
	return;
	//“Úwin10≤ª  ”√
	if( lstrcmpi( argv[1], TEXT("install")) == 0 )
	{
		SvcInstall();
		return;
	}
	else if( lstrcmpi( argv[1], TEXT("delete")) == 0 )
	{
		SvcDelete();
		return;
	}
	else if( lstrcmpi( argv[1], TEXT("start")) == 0 )
	{
		DoStartSvc(SVCNAME);
		return;
	}
	else if( lstrcmpi( argv[1], TEXT("stop")) == 0 )
	{
		DoStopSvc(SVCNAME);
		return;
	}
	else
	{
		printf("Usage:\n");
		printf("\tMyData [command]\n\n");
		printf("\t[command]\n");
		printf("\t  install\n");
		printf("\t  start\n");
		printf("\t  stop\n");
		printf("\t  delete\n");

	}
	SERVICE_TABLE_ENTRY DispatchTable[] = 
	{ 
		{ SVCNAME, (LPSERVICE_MAIN_FUNCTION) SvcMain }, 
		{ NULL, NULL } 
	}; 


	if (!StartServiceCtrlDispatcher( DispatchTable )) 
	{ 
		SvcReportEvent(TEXT("StartServiceCtrlDispatcher")); 
	} 
} 

VOID SvcInstall()
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	TCHAR szPath[MAX_PATH];

	if( !GetModuleFileName( NULL, szPath, MAX_PATH ) )
	{
		printf("Cannot install service (%d)\n", GetLastError());
		return;
	}

	// Get a handle to the SCM database. 

	schSCManager = OpenSCManager( 
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (NULL == schSCManager) 
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	// Create the service

	schService = CreateService( 
		schSCManager,              // SCM database 
		SVCNAME,                   // name of service 
		SVCNAME,                   // service name to display 
		SERVICE_ALL_ACCESS,        // desired access 
		SERVICE_WIN32_OWN_PROCESS, // service type 
		SERVICE_AUTO_START,      // start type 
		SERVICE_ERROR_NORMAL,      // error control type 
		szPath,                    // path to service's binary 
		NULL,                      // no load ordering group 
		NULL,                      // no tag identifier 
		NULL,                      // no dependencies 
		NULL,                      // LocalSystem account 
		NULL);                     // no password 

	if (schService == NULL) 
	{
		printf("CreateService failed (%d)\n", GetLastError()); 
		CloseServiceHandle(schSCManager);
		return;
	}
	else printf("Service installed successfully\n"); 

	CloseServiceHandle(schService); 
	CloseServiceHandle(schSCManager);
}

VOID WINAPI SvcMain( DWORD dwArgc, LPTSTR *lpszArgv )
{
	// Register the handler function for the service

	gSvcStatusHandle = RegisterServiceCtrlHandler( 
		SVCNAME, 
		SvcCtrlHandler);

	if( !gSvcStatusHandle )
	{ 
		SvcReportEvent(TEXT("RegisterServiceCtrlHandler")); 
		return; 
	} 

	gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS; 
	gSvcStatus.dwServiceSpecificExitCode = 0;    

	ReportSvcStatus( SERVICE_START_PENDING, NO_ERROR, 3000 );

	SvcInit( dwArgc, lpszArgv );
}

VOID SvcInit( DWORD dwArgc, LPTSTR *lpszArgv)
{
	ghSvcStopEvent = CreateEvent(
		NULL,    // default security attributes
		TRUE,    // manual reset event
		FALSE,   // not signaled
		NULL);   // no name

	if ( ghSvcStopEvent == NULL)
	{
		ReportSvcStatus( SERVICE_STOPPED, NO_ERROR, 0 );
		return;
	}

	ReportSvcStatus( SERVICE_RUNNING, NO_ERROR, 0 );

	// TO_DO: Perform work until service stops.
	HANDLE thandle = (HANDLE)_beginthreadex( NULL, 0, MyDataMain, 0, 0, NULL);
	if (thandle == 0)
	{
		ReportSvcStatus( SERVICE_RUNNING, ERROR_INVALID_HANDLE, 0 );
		return ;
	}
	else
		CloseHandle(thandle);
	while(1)
	{
		WaitForSingleObject(ghSvcStopEvent, INFINITE);
		_dm.StopRunning();
		EndEventLoop();
		ReportSvcStatus( SERVICE_STOPPED, NO_ERROR, 0 );
		return;
	}
}

VOID ReportSvcStatus( DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwWaitHint)
{
	static DWORD dwCheckPoint = 1;

	// Fill in the SERVICE_STATUS structure.

	gSvcStatus.dwCurrentState = dwCurrentState;
	gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
	gSvcStatus.dwWaitHint = dwWaitHint;

	if (dwCurrentState == SERVICE_START_PENDING)
		gSvcStatus.dwControlsAccepted = 0;
	else gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	if ( (dwCurrentState == SERVICE_RUNNING) ||
		(dwCurrentState == SERVICE_STOPPED) )
		gSvcStatus.dwCheckPoint = 0;
	else gSvcStatus.dwCheckPoint = dwCheckPoint++;

	// Report the status of the service to the SCM.
	SetServiceStatus( gSvcStatusHandle, &gSvcStatus );
}

VOID WINAPI SvcCtrlHandler( DWORD dwCtrl )
{
	// Handle the requested control code. 

	switch(dwCtrl) 
	{  
	case SERVICE_CONTROL_STOP: 
		ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

		// Signal the service to stop.

		SetEvent(ghSvcStopEvent);
		ReportSvcStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);

		return;

	case SERVICE_CONTROL_INTERROGATE: 
		break; 

	default: 
		break;
	} 

}

VOID SvcReportEvent(LPTSTR szFunction) 
{ 
	HANDLE hEventSource;
	LPCTSTR lpszStrings[2];
	TCHAR Buffer[80];

	hEventSource = RegisterEventSource(NULL, SVCNAME);

	if( NULL != hEventSource )
	{
		StringCchPrintf(Buffer, 80, TEXT("%s failed with %d"), szFunction, GetLastError());

		lpszStrings[0] = SVCNAME;
		lpszStrings[1] = Buffer;

		ReportEvent(hEventSource,        // event log handle
			EVENTLOG_ERROR_TYPE, // event type
			0,                   // event category
			SVC_ERROR,           // event identifier
			NULL,                // no security identifier
			2,                   // size of lpszStrings array
			0,                   // no binary data
			lpszStrings,         // array of strings
			NULL);               // no binary data

		DeregisterEventSource(hEventSource);
	}
}

VOID SvcDelete()
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;

	// Get a handle to the SCM database. 

	schSCManager = OpenSCManager( 
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (NULL == schSCManager) 
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	// Get a handle to the service.

	schService = OpenService( 
		schSCManager,       // SCM database 
		SVCNAME,          // name of service 
		DELETE);            // need delete access 

	if (schService == NULL)
	{ 
		printf("OpenService failed (%d)\n", GetLastError()); 
		CloseServiceHandle(schSCManager);
		return;
	}

	// Delete the service.

	if (! DeleteService(schService) ) 
	{
		printf("DeleteService failed (%d)\n", GetLastError()); 
	}
	else printf("Service deleted successfully\n"); 

	CloseServiceHandle(schService); 
	CloseServiceHandle(schSCManager);
}

VOID __stdcall DoStartSvc(char* szSvcName)
{
	SERVICE_STATUS_PROCESS ssStatus; 
	DWORD dwOldCheckPoint; 
	DWORD dwStartTickCount;
	DWORD dwWaitTime;
	DWORD dwBytesNeeded;

	// Get a handle to the SCM database. 

	schSCManager = OpenSCManager( 
		NULL,                    // local computer
		NULL,                    // servicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (NULL == schSCManager) 
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	// Get a handle to the service.

	schService = OpenService( 
		schSCManager,         // SCM database 
		szSvcName,            // name of service 
		SERVICE_ALL_ACCESS);  // full access 

	if (schService == NULL)
	{ 
		printf("OpenService failed (%d)\n", GetLastError()); 
		CloseServiceHandle(schSCManager);
		return;
	}    

	// Check the status in case the service is not stopped. 

	if (!QueryServiceStatusEx( 
		schService,                     // handle to service 
		SC_STATUS_PROCESS_INFO,         // information level
		(LPBYTE) &ssStatus,             // address of structure
		sizeof(SERVICE_STATUS_PROCESS), // size of structure
		&dwBytesNeeded ) )              // size needed if buffer is too small
	{
		printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
		CloseServiceHandle(schService); 
		CloseServiceHandle(schSCManager);
		return; 
	}

	// Check if the service is already running. It would be possible 
	// to stop the service here, but for simplicity this example just returns. 

	if(ssStatus.dwCurrentState != SERVICE_STOPPED && ssStatus.dwCurrentState != SERVICE_STOP_PENDING)
	{
		printf("Cannot start the service because it is already running\n");
		CloseServiceHandle(schService); 
		CloseServiceHandle(schSCManager);
		return; 
	}

	// Save the tick count and initial checkpoint.

	dwStartTickCount = GetTickCount();
	dwOldCheckPoint = ssStatus.dwCheckPoint;

	// Wait for the service to stop before attempting to start it.

	while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
	{
		// Do not wait longer than the wait hint. A good interval is 
		// one-tenth of the wait hint but not less than 1 second  
		// and not more than 10 seconds. 

		dwWaitTime = ssStatus.dwWaitHint / 10;

		if( dwWaitTime < 1000 )
			dwWaitTime = 1000;
		else if ( dwWaitTime > 10000 )
			dwWaitTime = 10000;

		Sleep( dwWaitTime );

		// Check the status until the service is no longer stop pending. 

		if (!QueryServiceStatusEx( 
			schService,                     // handle to service 
			SC_STATUS_PROCESS_INFO,         // information level
			(LPBYTE) &ssStatus,             // address of structure
			sizeof(SERVICE_STATUS_PROCESS), // size of structure
			&dwBytesNeeded ) )              // size needed if buffer is too small
		{
			printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
			CloseServiceHandle(schService); 
			CloseServiceHandle(schSCManager);
			return; 
		}

		if ( ssStatus.dwCheckPoint > dwOldCheckPoint )
		{
			// Continue to wait and check.

			dwStartTickCount = GetTickCount();
			dwOldCheckPoint = ssStatus.dwCheckPoint;
		}
		else
		{
			if(GetTickCount()-dwStartTickCount > ssStatus.dwWaitHint)
			{
				printf("Timeout waiting for service to stop\n");
				CloseServiceHandle(schService); 
				CloseServiceHandle(schSCManager);
				return; 
			}
		}
	}

	// Attempt to start the service.

	if (!StartService(
		schService,  // handle to service 
		0,           // number of arguments 
		NULL) )      // no arguments 
	{
		printf("StartService failed (%d)\n", GetLastError());
		CloseServiceHandle(schService); 
		CloseServiceHandle(schSCManager);
		return; 
	}
	else printf("Service start pending...\n"); 

	// Check the status until the service is no longer start pending. 

	if (!QueryServiceStatusEx( 
		schService,                     // handle to service 
		SC_STATUS_PROCESS_INFO,         // info level
		(LPBYTE) &ssStatus,             // address of structure
		sizeof(SERVICE_STATUS_PROCESS), // size of structure
		&dwBytesNeeded ) )              // if buffer too small
	{
		printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
		CloseServiceHandle(schService); 
		CloseServiceHandle(schSCManager);
		return; 
	}

	// Save the tick count and initial checkpoint.

	dwStartTickCount = GetTickCount();
	dwOldCheckPoint = ssStatus.dwCheckPoint;

	while (ssStatus.dwCurrentState == SERVICE_START_PENDING) 
	{ 
		// Do not wait longer than the wait hint. A good interval is 
		// one-tenth the wait hint, but no less than 1 second and no 
		// more than 10 seconds. 

		dwWaitTime = ssStatus.dwWaitHint / 10;

		if( dwWaitTime < 1000 )
			dwWaitTime = 1000;
		else if ( dwWaitTime > 10000 )
			dwWaitTime = 10000;

		Sleep( dwWaitTime );

		// Check the status again. 

		if (!QueryServiceStatusEx( 
			schService,             // handle to service 
			SC_STATUS_PROCESS_INFO, // info level
			(LPBYTE) &ssStatus,             // address of structure
			sizeof(SERVICE_STATUS_PROCESS), // size of structure
			&dwBytesNeeded ) )              // if buffer too small
		{
			printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
			break; 
		}

		if ( ssStatus.dwCheckPoint > dwOldCheckPoint )
		{
			// Continue to wait and check.

			dwStartTickCount = GetTickCount();
			dwOldCheckPoint = ssStatus.dwCheckPoint;
		}
		else
		{
			if(GetTickCount()-dwStartTickCount > ssStatus.dwWaitHint)
			{
				// No progress made within the wait hint.
				break;
			}
		}
	} 

	// Determine whether the service is running.

	if (ssStatus.dwCurrentState == SERVICE_RUNNING) 
	{
		printf("Service started successfully.\n"); 
	}
	else 
	{ 
		printf("Service not started. \n");
		printf("  Current State: %d\n", ssStatus.dwCurrentState); 
		printf("  Exit Code: %d\n", ssStatus.dwWin32ExitCode); 
		printf("  Check Point: %d\n", ssStatus.dwCheckPoint); 
		printf("  Wait Hint: %d\n", ssStatus.dwWaitHint); 
	} 

	CloseServiceHandle(schService); 
	CloseServiceHandle(schSCManager);
}
VOID __stdcall DoStopSvc(char* szSvcName)
{
	SERVICE_STATUS_PROCESS ssp;
	DWORD dwStartTime = GetTickCount();
	DWORD dwBytesNeeded;
	DWORD dwTimeout = 30000; // 30-second time-out
	DWORD dwWaitTime;

	// Get a handle to the SCM database. 

	schSCManager = OpenSCManager( 
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (NULL == schSCManager) 
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	// Get a handle to the service.

	schService = OpenService( 
		schSCManager,         // SCM database 
		szSvcName,            // name of service 
		SERVICE_STOP | 
		SERVICE_QUERY_STATUS | 
		SERVICE_ENUMERATE_DEPENDENTS);  

	if (schService == NULL)
	{ 
		printf("OpenService failed (%d)\n", GetLastError()); 
		CloseServiceHandle(schSCManager);
		return;
	}    

	// Make sure the service is not already stopped.

	if ( !QueryServiceStatusEx( 
		schService, 
		SC_STATUS_PROCESS_INFO,
		(LPBYTE)&ssp, 
		sizeof(SERVICE_STATUS_PROCESS),
		&dwBytesNeeded ) )
	{
		printf("QueryServiceStatusEx failed (%d)\n", GetLastError()); 
		goto stop_cleanup;
	}

	if ( ssp.dwCurrentState == SERVICE_STOPPED )
	{
		printf("Service is already stopped.\n");
		goto stop_cleanup;
	}

	// If a stop is pending, wait for it.

	while ( ssp.dwCurrentState == SERVICE_STOP_PENDING ) 
	{
		printf("Service stop pending...\n");

		// Do not wait longer than the wait hint. A good interval is 
		// one-tenth of the wait hint but not less than 1 second  
		// and not more than 10 seconds. 

		dwWaitTime = ssp.dwWaitHint / 10;

		if( dwWaitTime < 1000 )
			dwWaitTime = 1000;
		else if ( dwWaitTime > 10000 )
			dwWaitTime = 10000;

		Sleep( dwWaitTime );

		if ( !QueryServiceStatusEx( 
			schService, 
			SC_STATUS_PROCESS_INFO,
			(LPBYTE)&ssp, 
			sizeof(SERVICE_STATUS_PROCESS),
			&dwBytesNeeded ) )
		{
			printf("QueryServiceStatusEx failed (%d)\n", GetLastError()); 
			goto stop_cleanup;
		}

		if ( ssp.dwCurrentState == SERVICE_STOPPED )
		{
			printf("Service stopped successfully.\n");
			goto stop_cleanup;
		}

		if ( GetTickCount() - dwStartTime > dwTimeout )
		{
			printf("Service stop timed out.\n");
			goto stop_cleanup;
		}
	}

	// If the service is running, dependencies must be stopped first.

	StopDependentServices();

	// Send a stop code to the service.

	if ( !ControlService( 
		schService, 
		SERVICE_CONTROL_STOP, 
		(LPSERVICE_STATUS) &ssp ) )
	{
		printf( "ControlService failed (%d)\n", GetLastError() );
		goto stop_cleanup;
	}

	// Wait for the service to stop.

	while ( ssp.dwCurrentState != SERVICE_STOPPED ) 
	{
		Sleep( ssp.dwWaitHint );
		if ( !QueryServiceStatusEx( 
			schService, 
			SC_STATUS_PROCESS_INFO,
			(LPBYTE)&ssp, 
			sizeof(SERVICE_STATUS_PROCESS),
			&dwBytesNeeded ) )
		{
			printf( "QueryServiceStatusEx failed (%d)\n", GetLastError() );
			goto stop_cleanup;
		}

		if ( ssp.dwCurrentState == SERVICE_STOPPED )
			break;

		if ( GetTickCount() - dwStartTime > dwTimeout )
		{
			printf( "Wait timed out\n" );
			goto stop_cleanup;
		}
	}
	printf("Service stopped successfully\n");

stop_cleanup:
	CloseServiceHandle(schService); 
	CloseServiceHandle(schSCManager);
}

BOOL __stdcall StopDependentServices()
{
	DWORD i;
	DWORD dwBytesNeeded;
	DWORD dwCount;

	LPENUM_SERVICE_STATUS   lpDependencies = NULL;
	ENUM_SERVICE_STATUS     ess;
	SC_HANDLE               hDepService;
	SERVICE_STATUS_PROCESS  ssp;

	DWORD dwStartTime = GetTickCount();
	DWORD dwTimeout = 30000; // 30-second time-out

	// Pass a zero-length buffer to get the required buffer size.
	if ( EnumDependentServices( schService, SERVICE_ACTIVE, 
		lpDependencies, 0, &dwBytesNeeded, &dwCount ) ) 
	{
		// If the Enum call succeeds, then there are no dependent
		// services, so do nothing.
		return TRUE;
	} 
	else 
	{
		if ( GetLastError() != ERROR_MORE_DATA )
			return FALSE; // Unexpected error

		// Allocate a buffer for the dependencies.
		lpDependencies = (LPENUM_SERVICE_STATUS) HeapAlloc( 
			GetProcessHeap(), HEAP_ZERO_MEMORY, dwBytesNeeded );

		if ( !lpDependencies )
			return FALSE;

		__try {
			// Enumerate the dependencies.
			if ( !EnumDependentServices( schService, SERVICE_ACTIVE, 
				lpDependencies, dwBytesNeeded, &dwBytesNeeded,
				&dwCount ) )
				return FALSE;

			for ( i = 0; i < dwCount; i++ ) 
			{
				ess = *(lpDependencies + i);
				// Open the service.
				hDepService = OpenService( schSCManager, 
					ess.lpServiceName, 
					SERVICE_STOP | SERVICE_QUERY_STATUS );

				if ( !hDepService )
					return FALSE;

				__try {
					// Send a stop code.
					if ( !ControlService( hDepService, 
						SERVICE_CONTROL_STOP,
						(LPSERVICE_STATUS) &ssp ) )
						return FALSE;

					// Wait for the service to stop.
					while ( ssp.dwCurrentState != SERVICE_STOPPED ) 
					{
						Sleep( ssp.dwWaitHint );
						if ( !QueryServiceStatusEx( 
							hDepService, 
							SC_STATUS_PROCESS_INFO,
							(LPBYTE)&ssp, 
							sizeof(SERVICE_STATUS_PROCESS),
							&dwBytesNeeded ) )
							return FALSE;

						if ( ssp.dwCurrentState == SERVICE_STOPPED )
							break;

						if ( GetTickCount() - dwStartTime > dwTimeout )
							return FALSE;
					}
				} 
				__finally 
				{
					// Always release the service handle.
					CloseServiceHandle( hDepService );
				}
			}
		} 
		__finally 
		{
			// Always free the enumeration buffer.
			HeapFree( GetProcessHeap(), 0, lpDependencies );
		}
	} 
	return TRUE;
}

