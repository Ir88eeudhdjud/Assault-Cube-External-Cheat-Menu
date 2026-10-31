#ifndef CHANGE_AC_ADDR
#define CHANGE_AC_ADDR

#include <stdio.h>
#include <stdint.h>
#include <windows.h>
#include <inttypes.h>
#include <memoryapi.h>
#include <errhandlingapi.h>
#include <processthreadsapi.h>
#include "D:/CLanguage/CustomHeaders/GetPIDByProcessName.h"
#include "D:/CLanguage/CustomHeaders/GetMDBaseAddrByProgNm.h"

void ChangeAddrOffVal(HWND hProcess, uintptr_t mdBaseAddr, uintptr_t offset, int32_t val)
{
	uintptr_t baseAddr = mdBaseAddr + 0x17E0A8;
	
	DWORD oldPrt;

	uint32_t BAFP; // base addr from ptr
	ReadProcessMemory(hProcess, (LPCVOID)baseAddr, &BAFP, sizeof(BAFP), NULL);

	uintptr_t addr = BAFP + offset;
	WriteProcessMemory(hProcess, (LPVOID)addr, &val, sizeof(val), NULL);
}

void ChangeAddrOffValArray(HWND hProcess, uintptr_t mdBaseAddr, uintptr_t offsets[], int arrayL, int32_t val)
{
	uintptr_t baseAddr = mdBaseAddr + 0x17E0A8;

	uint32_t BAFP; // base addr from ptr
	ReadProcessMemory(hProcess, (LPCVOID)baseAddr, &BAFP, sizeof(BAFP), NULL);

	for (int i=0; i<arrayL; i++)
	{
		uintptr_t addr = BAFP + offsets[i];
		WriteProcessMemory(hProcess, (LPVOID)addr, &val, sizeof(val), NULL);
	}
}

void ChangeAddrVal(HWND hProcess, uintptr_t* addr, int arr_sz, size_t sz, int32_t* val)
{
	for (int i=0; i<arr_sz; i++) {
		WriteProcessMemory(hProcess, (LPVOID)addr[i], &val[i], sz, NULL);
	}
}

#endif