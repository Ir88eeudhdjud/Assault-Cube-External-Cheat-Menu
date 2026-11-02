#ifndef GET_PID_BY_PROCESS_NAME
#define GET_PID_BY_PROCESS_NAME

#include <windows.h>
#include <tlhelp32.h>
#include <handleapi.h>

// returns 0 if did not found, returns 1 on error

DWORD get_pid_by_process_name(const char *processName)
{
	DWORD pid;
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	
	if (hSnapShot == INVALID_HANDLE_VALUE) {
		printf("Could not create snapshot of the processes, err: %lu\n", GetLastError());
		return 1;
	}
	
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);
	
	if (Process32First(hSnapShot, &pe32) == 0) {
		printf("Could not get first process in the list, err: %lu\n", GetLastError());
		CloseHandle(hSnapShot);
		return 1;
	}

	do {
		if (_stricmp(pe32.szExeFile, processName) == 0) {
			if (pe32.th32ProcessID != GetCurrentProcessId()) {
				pid = pe32.th32ProcessID;
			}
			break;
		}
	} while (Process32Next(hSnapShot, &pe32));

	CloseHandle(hSnapShot);
	return pid;
}

#endif
