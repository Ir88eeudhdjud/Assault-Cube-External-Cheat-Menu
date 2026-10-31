#ifndef TERMINATE_ADDRESS_INSTRUCTION
#define TERMINATE_ADDRESS_INSTRUCTION

#include <inttypes.h>
#include <memoryapi.h>
#include <errhandlingapi.h>
#include <processthreadsapi.h>
#include <string.h>

// retOGBytes will return bytes that it first was when RdPrcMmr read the addr, to return functionality of addr
uint64_t* TermAddrFunc(HANDLE hProcess, uintptr_t* addr, int arr_sz, const size_t sz, int retBytes)
{
	// allocate memory space for ogBytes, [0] - is const size_t sz, the rest is bytes
	uint64_t* ogBytes = (uint64_t *)malloc(((arr_sz*sz)+1)*sizeof(uint64_t));

	if (retBytes == 1)
	{
		if (ogBytes != NULL)
		{
			for (int i=0; i<arr_sz; i++) {
				DWORD oldPrt;
				VirtualProtectEx(hProcess, (LPVOID)addr[i], sz, PAGE_READWRITE, &oldPrt);

				uint64_t* ptr = ogBytes + (i * sz);
				ReadProcessMemory(hProcess, (LPCVOID)addr[i], ptr, sz, NULL);

				VirtualProtectEx(hProcess, (LPVOID)addr[i], sz, oldPrt, &oldPrt);
			}
			memmove(&ogBytes[1], &ogBytes[0], (arr_sz*sz)*sizeof(uint64_t));
			ogBytes[0] = sz;
		} else {printf("Could not allocate memory in TermAddrFunc, err: %lu\n", GetLastError());}
	}

	/*printf("\n--full og bytes buf--\n");
	for (int i=0; i<((arr_sz*sz)+1)*sizeof(uint8_t); i++) {
		printf("  %"PRIxPTR"\n", ogBytes[i]);
	}*/

	/*printf("\n--ar_sz--\n");
	printf("%d\n", arr_sz);
	printf("--sz--\n");
	printf("%lu\n", sz);*/
	// write to a buf
	uint8_t nopBuf[sz];
	// 0x90 bytes in processor means - do no operation in that addr
	memset(nopBuf, 0x90, sz);

	// We have buf with n nops, but why does WrtPrcMmr not writes it in one address, but n addresses?
	// When we pass addr, this addr is just a point of start, and SIZE nSize dictates in how many addrs
	// to write in, based on 'tis own size
	for (int i=0; i<arr_sz; i++) {
		DWORD oldPrt;
		VirtualProtectEx(hProcess, (LPVOID)addr[i], sz, PAGE_READWRITE, &oldPrt);

		WriteProcessMemory(hProcess, (LPVOID)addr[i], nopBuf, sz, NULL);

		VirtualProtectEx(hProcess, (LPVOID)addr[i], sz, oldPrt, &oldPrt);
	}

	if (retBytes == 0) {free(ogBytes); return 0;}
	return ogBytes;
}

void RestoreAddrFunc(HANDLE hProcess, HANDLE hThread, uintptr_t* addr, int arr_sz, uint64_t* buf, const size_t sz, int rem_sz_from_buf)
{
	if (!SuspendThread(hThread) && GetLastError() != 0) printf("Could not suspend thread in RestoreAddrFunc, err: %lu\n", GetLastError());

	if (rem_sz_from_buf == 1) {
		memmove(&buf[0], &buf[1], (arr_sz*sz)*sizeof(uint8_t));
	}
	/*printf("\n--restore_buf--\n");
	for (int i=0; i<(arr_sz*sz)*sizeof(uint8_t); i++) {
		printf("  %"PRIxPTR"\n", buf[i]);
	}*/
	
	for (int i=0; i<arr_sz; i++) {
		DWORD oldPrt;
		if (VirtualProtectEx(hProcess, (LPVOID)addr[i], sz, PAGE_READWRITE, &oldPrt) == 0) {

			uint8_t* local_buf = malloc(sz*sizeof(uint8_t));
			memcpy(local_buf, buf, sz*sizeof(uint8_t)); // grabs only needed num(sz) of bytes

			/*printf("\n--local_buf--\n");
			for (int i2=0; i2<sz; i2++) {
				printf("  %"PRIxPTR"\n", local_buf[i2]);
			}*/

			WriteProcessMemory(hProcess, (LPVOID)addr[i], local_buf, sz, NULL);

			memmove(&buf[0], &buf[sz], (arr_sz*sz)*sizeof(uint8_t)); // skips first num of bytes
			free(local_buf);

			VirtualProtectEx(hProcess, (LPVOID)addr[i], sz, oldPrt, &oldPrt);

			int fRes = FlushInstructionCache(hProcess, (LPCVOID)addr[i], sz);
			if (fRes == 0) {
				printf("FlushInstructionCache, err: %lu\n", GetLastError());
			}
		} else printf("Could not change address protection to PAGE_READWRITE, err: %lu\n", GetLastError());
	}

	if (!ResumeThread(hThread)) printf("Could not resume thread in RestoreAddrFunc, err: %lu\n", GetLastError());
}

#endif