#ifndef GET_PID_BY_PROCESS_NAME
#define GET_PID_BY_PROCESS_NAME

#include <windows.h>
#include <tlhelp32.h> // CreateToolhelp32Snapshot, PROCESSENTRY32, Process32First, Process32Next
#include <handleapi.h> // CloseHandle

// returns 0 if did not found, returns 1 on error

// DWORD - unsigned 32x integer
DWORD get_pid_by_process_name(const char *processName)
{
	DWORD pid;
	// Takes snapshot. If flag is TH32CS_SNAPPROCESS, then set pid(second arg) to 0. It will
	// include all processes in system in snapshot
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	// On error, then returns INVALID_HANDLE_VALUE
	if (hSnapShot == INVALID_HANDLE_VALUE) {
		printf("Could not create snapshot of the processes, err: %lu\n", GetLastError());
		return 1;
	}

	// Set up PROCESSENTRY32 struct, func Process32First and Next will write data here
	PROCESSENTRY32 pe32;
	// The size of the structure in bytes
	pe32.dwSize = sizeof(PROCESSENTRY32);

	// Writes info into PROCESSENTRY32, about the first process it found in the system
	// Returns zero if could not copy data into our struct
	if (Process32First(hSnapShot, &pe32) == 0) {
		printf("Could not get first process in the list, err: %lu\n", GetLastError());
		CloseHandle(hSnapShot);
		return 1;
	}

	do {
		// _stricmp performs a case-insensitive comparison of strings, this means
		// it compares two strings, ignoring big or small char.
		// pe32.szExeFile - name of the exetuable file
		// Returns 0 if matched
		if (_stricmp(pe32.szExeFile, processName) == 0) {
			// Here our process ID
			if (pe32.th32ProcessID != GetCurrentProcessId()) {
				pid = pe32.th32ProcessID;
			}
			break;
		}
		// Writes info into PROCESSENTRY32, about the next process it found in the system
	} while (Process32Next(hSnapShot, &pe32));

	CloseHandle(hSnapShot);
	return pid;
}

#endif