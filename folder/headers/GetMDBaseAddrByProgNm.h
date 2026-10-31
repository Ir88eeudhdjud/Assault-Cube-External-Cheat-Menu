#ifndef GET_MD_BASE_ADDR_BY_PROG_NM
#define GET_MD_BASE_ADDR_BY_PROG_NM

#include <windows.h>
#include <tlhelp32.h> // CreateToolhelp32Snapshot, MODULEENTRY32, Module32First, Module32Next
#include <handleapi.h> // CloseHandle

uintptr_t get_module_base_addr(const char *mdName, DWORD pid)
{
	uintptr_t mdBaseAddr;

	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);

	if (hSnapShot == INVALID_HANDLE_VALUE) {
		printf("Could not create snapshot of the processes, err: %lu\n", GetLastError());
		return 1;
	}

	MODULEENTRY32 me32;
	me32.dwSize = sizeof(MODULEENTRY32);

	if (Module32First(hSnapShot, &me32) == 0) {
		printf("Could not get the first module in the list, err: %lu\n", GetLastError());
		CloseHandle(hSnapShot);
		return 1;
	}

	do {
		if (_stricmp(me32.szModule, mdName) == 0) {
			mdBaseAddr = (uintptr_t)me32.modBaseAddr;
			break;
		}
	} while (Module32Next(hSnapShot, &me32));

	CloseHandle(hSnapShot);
	return mdBaseAddr;
}

#endif