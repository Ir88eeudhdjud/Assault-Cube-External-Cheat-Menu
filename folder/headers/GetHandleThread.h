#ifndef GET_HANDLE_THREAD
#define GET_HANDLE_THREAD

#include <windows.h>
#include <tlhelp32.h>
#include <handleapi.h>

HANDLE get_hThread(DWORD pid) // NULL on error
{
	HANDLE hThread = NULL;

	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

	if (hSnapShot == INVALID_HANDLE_VALUE) {
		printf("Could not create snapshot of the processes, err: %lu\n", GetLastError());
		return NULL;
	}

	THREADENTRY32 te32 = {};
	te32.dwSize = sizeof(THREADENTRY32);

	if (Thread32First(hSnapShot, &te32) == 0) {
		printf("Could not get the first hThread in the list, err: %lu\n", GetLastError());
		CloseHandle(hSnapShot);
		return NULL;
	}

	do {
		if (te32.th32OwnerProcessID == pid) {
			hThread = OpenThread(THREAD_SUSPEND_RESUME, 0, te32.th32ThreadID);
			break;
		}
	} while (Thread32Next(hSnapShot, &te32));

	CloseHandle(hSnapShot);
	return hThread;
}

#endif