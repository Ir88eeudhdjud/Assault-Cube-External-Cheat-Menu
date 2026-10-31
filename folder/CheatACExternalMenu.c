#include <stdio.h>
#include <windows.h>
#include <winuser.h>
#include <stdint.h>
#include <unistd.h>
#include <processthreadsapi.h>
#include <inttypes.h>
#include <errhandlingapi.h>
#include <memoryapi.h>
#include <pthread.h>
#include "Headers/ChangeACAddr.h"
#include "D:/CLanguage/CustomHeaders/GetPIDByProcessName.h"
#include "D:\CLanguage\CustomHeaders\GetMDBaseAddrByProgNm.h"
#include "D:\CLanguage\CustomHeaders\GetHandleThread.h"
#include "D:/CLanguage/Projects/HacksWinAPI/Headers/TerminateAddressInstruction.h"
#include <stdbool.h>
#include <commctrl.h>
#include <math.h>

#define PI 3.14159265358979323846f

void SH_Console_Window(int val)
{
	HWND hConsole = GetConsoleWindow();
	
	if (hConsole == NULL)
	{
		printf("hConsole is NULL\n");
		return;
	}

	ShowWindow(hConsole, val);
}

void SH_console_func(HWND SHCnslBtn)
{
	int val;

	if (IsWindowVisible(GetConsoleWindow()))
	{
		val = 0;
		SetWindowTextW(SHCnslBtn, L"Show console");
	} else {
		val = 1;
		SetWindowTextW(SHCnslBtn, L"Hide console");
	}
	
	SH_Console_Window(val);
}

HANDLE GetProcess(DWORD pid)
{
    return OpenProcess(
        PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_SET_INFORMATION,
        FALSE, 
        pid
    );
}

#define ID_1_BUTTON 10
#define ID_2_BUTTON 11
#define ID_3_BUTTON 12
#define ID_4_BUTTON 13
#define ID_5_BUTTON 14
#define ID_6_BUTTON 15
#define ID_7_BUTTON 16
#define ID_8_BUTTON 17
#define ID_9_BUTTON 18
#define ID_10_BUTTON 19
#define ID_11_BUTTON 20
#define ID_12_BUTTON 21
#define ID_13_BUTTON 22

struct term_addr_bt_data {
	uint64_t* arr[1000];
};

struct _keepcycle_addroffsets {
	uintptr_t offset_arr[100];
	int value_arr[100]; // value to an address sits at the same position as address in offset_arr
};

static struct term_addr_bt_data _bytes_data;
static struct _keepcycle_addroffsets _addroffsets;

pthread_t keepcycle_thread;

HANDLE hProcess; // closed in WindowProc -> WM_DESTROY
HANDLE hThread; // closed in WIndowProc -> WM_DESTROY
uintptr_t mdBaseAddr;

DWORD pid;
DWORD up_t = 1000;

uintptr_t AmmoOffsets[] = {0x140, 0x12C, 0x138, 0x13C, 0x134, 0x130, 0x148};
size_t AmmoArrSize = sizeof(AmmoOffsets) / sizeof(AmmoOffsets[0]);

int aimbot = 0; // -> RBDownSetUp
int NumOfEnts = 0;

struct Player_s {
	uint32_t baseAddr;
	uint32_t gmBaseAddr; // game mode base addr
	int32_t GameMode;
	int32_t Team;
	int32_t Health;
	float Pos[3];
	char Name[32];
};

void PlayerReadMemory(struct Player_s* p)
{
    if (!p->baseAddr) {
    	ReadProcessMemory(hProcess, (LPCVOID)(uintptr_t)(mdBaseAddr+0x17E0A8), &(p->baseAddr), sizeof(p->baseAddr), NULL);
    }
    ReadProcessMemory(hProcess, (LPCVOID)(uintptr_t)(p->baseAddr+0x30C), &(p->Team), sizeof(p->Team), NULL);
    ReadProcessMemory(hProcess, (LPCVOID)(uintptr_t)(p->baseAddr+0xEC), &(p->Health), sizeof(p->Health), NULL);
    ReadProcessMemory(hProcess, (LPCVOID)(uintptr_t)(p->baseAddr+0x28), &(p->Pos), sizeof(p->Pos), NULL);
    ReadProcessMemory(hProcess, (LPCVOID)(uintptr_t)(p->baseAddr+0x205), &(p->Name), sizeof(p->Name), NULL);

    if (!p->gmBaseAddr) {
    	ReadProcessMemory(hProcess, (LPCVOID)(uintptr_t)(mdBaseAddr+0x05C7A8), &(p->gmBaseAddr), sizeof(p->gmBaseAddr), NULL);
    }
    ReadProcessMemory(hProcess, (LPCVOID)(uintptr_t)(p->gmBaseAddr), &(p->GameMode), sizeof(p->GameMode), NULL);
}

struct NearestEnt_s { // Only Enemy
	uint32_t NumOfEntsBaseAddr;
	uint32_t CurrEntBaseAddr;
	uint32_t EnemyAddrs[32];
	float Pos[3];
};

struct Player_s Player;
struct NearestEnt_s NearestEnt;

void EntityReadMemory(struct NearestEnt_s* e)
{
	if (!e->NumOfEntsBaseAddr) // base addr for number of ents
		ReadProcessMemory(hProcess, (LPCVOID)(uintptr_t)(mdBaseAddr+0x05C434), &(e->NumOfEntsBaseAddr), sizeof(e->NumOfEntsBaseAddr), NULL);
	// read num of ents
	ReadProcessMemory(hProcess, (LPCVOID)(uintptr_t)(e->NumOfEntsBaseAddr), &NumOfEnts, sizeof(NumOfEnts), NULL);
	NumOfEnts -= 1; // do not count local player

	if (!e->CurrEntBaseAddr) { // base addr for ent list
		ReadProcessMemory(hProcess, (LPCVOID)(uintptr_t)(mdBaseAddr+0x191FCC), &(e->CurrEntBaseAddr), sizeof(e->CurrEntBaseAddr), NULL);
		for (int i=0; i<32; i++) {
			e->EnemyAddrs[i] = 0;
		}
	}
	
	int32_t EntityHealth[NumOfEnts];
	float EntityPos[NumOfEnts*3];

	u_int alive = 0;
	
	for (int i=0; i<NumOfEnts; i++) {
		if (e->EnemyAddrs[i] == 0) { // if EnemyAddrs slot is empty, then we write into EnemyAddrs
			uintptr_t addr = e->CurrEntBaseAddr+(i*4);
			ReadProcessMemory(hProcess, (LPCVOID)addr, &e->EnemyAddrs[i], sizeof(e->EnemyAddrs[i]), NULL);
		}

		int32_t health;
		int32_t team;
		ReadProcessMemory(hProcess, (LPCVOID)(uintptr_t)(e->EnemyAddrs[i]+0xEC), &health, sizeof(health), NULL);

		if (health > 0) {
			if (Player.GameMode == 7 || Player.GameMode == 20 || Player.GameMode == 21) { // game modes where teams are extant, if teams are inextant, then skip
				ReadProcessMemory(hProcess, (LPCVOID)(uintptr_t)(e->EnemyAddrs[i]+0x30C), &team, sizeof(team), NULL);
				if (team == Player.Team) continue; // if we are in the same team: skip
			}

			EntityHealth[alive] = health;
			ReadProcessMemory(hProcess, (LPCVOID)(uintptr_t)(e->EnemyAddrs[i]+0x28), &EntityPos[alive*3], sizeof(float)*3, NULL);

			alive += 1;
		}
	}

	if (alive == 0) return;
	
	float dist = 999999.000f;
	for (int i=0; i<alive; i++) {
		float deltaPos[3];
		
		for (int _i=0; _i<3; _i++) {
			int p = (i*3)+_i;
			deltaPos[_i] = EntityPos[p]-Player.Pos[_i];
		}
		
		float l_dist = sqrt(deltaPos[0]*deltaPos[0] + deltaPos[1]*deltaPos[1]);
		if (l_dist < dist) {
			dist = l_dist;
			for (int _i=0; _i<3; _i++) {
				e->Pos[_i] = deltaPos[_i];
			}
		}
	}
}

void W_KeepCycle_AddrOff(uintptr_t off, int32_t val) // writes into _keepcycle_addroffsets
{
	for (int i=0; i<100; i++) {
		if (_addroffsets.offset_arr[i] == 0) {
			_addroffsets.offset_arr[i] = off;
			_addroffsets.value_arr[i] = val;

			break;
		}
	}
}

void W_Array_KeepCycle_AddrOff(uintptr_t* off_arr, size_t sz, int32_t val)
{
	int _i = 0;

	for (int i=0; i<100; i++) {
		if (_addroffsets.offset_arr[i] == 0) {
			_addroffsets.offset_arr[i] = off_arr[_i];
			_addroffsets.value_arr[i] = val;

			if (_i == sz) break;
			_i++;
		}
	}
}

// Returns 0 if did not found address, 1 if did found and freed
int F_KeepCycle_AddrOff(uintptr_t off) // searchess for the address and value by your given address, and frees it(sets to 0)
{
	int retVal = 0;

	for (int i=0; i<100; i++) {
		if (_addroffsets.offset_arr[i] == off) {
			_addroffsets.offset_arr[i] = 0;
			_addroffsets.value_arr[i] = 0;

			retVal = 1;
			break;
		}
	}

	return retVal;
}

// ret 2 - freed not every offset, ret 1 - freed every offset
int F_Array_KeepCycle_AddrOff(uintptr_t* off_arr, size_t sz)
{
	int retVal = 0;
	int _i = 0;

	for (int i=0; i<100; i++) {
		if (_addroffsets.offset_arr[i] == off_arr[_i]) {
			_addroffsets.offset_arr[i] = 0;
			_addroffsets.value_arr[i] = 0;

			if (_i == sz) {
				retVal = 1;
				break;
			} else retVal = 2;
			_i++;
		}
	}

	return retVal;
}

HWND CreateButton(HWND hwnd, const wchar_t* txt, int px, int py, int w, int h, int ID);

HWND CreateTxtBoxOrStcTxt(HWND hwnd, const wchar_t* className, const wchar_t* txt, int px, int py, int w, int h, int ID);

HWND CreateCheckBox(HWND hwnd, const wchar_t* txt, int px, int py, int w, int h, int ID);

HWND CreateTrackbar(HWND hwnd, const wchar_t* txt, int px, int py, int w, int h, int ID); // build with -lcomctl32

void ChangeVal(int isArr, HWND hwnd, uintptr_t off, uintptr_t arr[], size_t arrSize)
{
	wchar_t buf[256];
	int32_t val;
	
	GetWindowTextW(hwnd, buf, 256);

	swscanf(buf, L"%" SCNd32, &val);

	if (isArr == 0)
	{
		ChangeAddrOffVal(hProcess, mdBaseAddr, off, val);
		return;
	}

	ChangeAddrOffValArray(hProcess, mdBaseAddr, arr, arrSize, val);
}

void TermAddrFuncCheckBox(HWND hwnd, uintptr_t* addr, int arr_sz, int sz, int id)
{
	if (SendMessage(hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED)
	{
		uint64_t* buf = TermAddrFunc(hProcess, addr, arr_sz, sz, 1);
		_bytes_data.arr[id] = buf;

		free(buf);
	} else if (SendMessage(hwnd, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
	{
		RestoreAddrFunc(hProcess, hThread, addr, arr_sz, _bytes_data.arr[id], _bytes_data.arr[id][0], 1);
		_bytes_data.arr[id] = NULL;
	}
}

// if not array set off_arr to 0, if array set off and off_arr to some array
void AddToKeepCycleCheckBox(HWND textHWND, HWND butHWND, uintptr_t off, uintptr_t* off_arr, size_t arrL)
{
	wchar_t buf[256];
	int32_t val;

	GetWindowTextW(textHWND, buf, 256);

	swscanf(buf, L"%" SCNd32, &val);
	
	if (SendMessage(butHWND, BM_GETCHECK, 0, 0) == BST_CHECKED)
	{
		if (off == 0) {W_Array_KeepCycle_AddrOff(off_arr, arrL, val); return;}
		W_KeepCycle_AddrOff(off, val);
	} else if (SendMessage(butHWND, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
	{
		if (off == 0) {F_Array_KeepCycle_AddrOff(off_arr, arrL); return;}
		F_KeepCycle_AddrOff(off);
	}
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HWND txtBoxHealthHWND;
	static HWND txtBoxAmmoHWND;

	static HWND SHCnslBtn; // console shw/hd button
	static HWND KeepCycleUpTime; // txt box

	static HWND ShtSpd; // shooting speed
	static HWND KnockBackHWND; // knockback
	static HWND CamKnockbackHWND; // cam knockback
	static HWND KnifeCdHWND; // kinfe cd
	static HWND AimbotHWND; // aimbot dawg

	static HWND KeepHealthCkBox;
	static HWND KeepAmmoCkBox;

	static HWND ChangeSpdTxtBoxHWND;

	switch (uMsg)
	{
	case WM_CREATE:
		txtBoxHealthHWND = CreateTxtBoxOrStcTxt(hwnd, L"EDIT", L"9999", 100, 10, 150, 20, 0); // health
		txtBoxAmmoHWND = CreateTxtBoxOrStcTxt(hwnd, L"EDIT", L"9999", 470, 10, 150, 20, 0); // ammo
		KeepCycleUpTime = CreateTxtBoxOrStcTxt(hwnd, L"EDIT", L"1000", 265, 350, 50, 25, ID_7_BUTTON); // keepcycle update time
		ChangeSpdTxtBoxHWND = CreateTxtBoxOrStcTxt(hwnd, L"EDIT", L"1", 135, 210, 40, 20, ID_12_BUTTON);

		CreateButton(hwnd, L"apply", 110, 40, 55, 20, ID_1_BUTTON); // health apply
		CreateButton(hwnd, L"apply", 480, 40, 55, 20, ID_2_BUTTON); // ammo apply
		CreateButton(hwnd, L"apply", 265, 320, 50, 25, ID_8_BUTTON); // keep cycle apply button
		KeepHealthCkBox = CreateCheckBox(hwnd, L"keep", 185, 40, 55, 20, ID_5_BUTTON); // keep health
		KeepAmmoCkBox = CreateCheckBox(hwnd, L"keep", 550, 40, 55, 20, ID_6_BUTTON); // keep ammo

		ShtSpd = CreateCheckBox(hwnd, L"Shooting speed", 10, 100, 125, 20, ID_4_BUTTON);
		KnockBackHWND = CreateCheckBox(hwnd, L"Knockback", 10, 125, 125, 20, ID_9_BUTTON);
		CamKnockbackHWND = CreateCheckBox(hwnd, L"Cam knockback", 10, 150, 125, 20, ID_10_BUTTON);
		KnifeCdHWND = CreateCheckBox(hwnd, L"Knife cd", 10, 175, 125, 20, ID_11_BUTTON);
		AimbotHWND = CreateCheckBox(hwnd, L"Aimbot", 140, 100, 125, 20, ID_13_BUTTON);

		CreateTxtBoxOrStcTxt(hwnd, L"STATIC", L"Health", 35, 10, 50, 20, 0); // healt text
		CreateTxtBoxOrStcTxt(hwnd, L"STATIC", L"Ammo", 405, 10, 50, 20, 0); // ammo text
		CreateTxtBoxOrStcTxt(hwnd, L"STATIC", L"keep cycle up_t", 150, 350, 110, 20, 0); // keep cycle up time text
		CreateTxtBoxOrStcTxt(hwnd, L"STATIC", L"ms", 320, 350, 20, 20, 0); // keep cycle up, text ms
		CreateTxtBoxOrStcTxt(hwnd, L"STATIC", L"Player is speed", 10, 210, 120, 20, 0);
		CreateTxtBoxOrStcTxt(hwnd, L"STATIC", L"RMB", 140, 125, 125, 20, 0);
		
		SHCnslBtn = CreateButton(hwnd, L"Hide Console", 30, 350, 100, 25, ID_3_BUTTON);

		CreateTrackbar(hwnd, L"speed", 180, 210, 120, 20, 0);
		
		return 0;
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED) {
			switch (LOWORD(wParam))
			{
			case ID_1_BUTTON:
				ChangeVal(0, txtBoxHealthHWND, 0xEC, 0, 0);
				return 0;
			case ID_2_BUTTON:
				ChangeVal(1, txtBoxAmmoHWND, 0, AmmoOffsets, AmmoArrSize);
				return 0;
			case ID_3_BUTTON:
				SH_console_func(SHCnslBtn);
				return 0;
			case ID_4_BUTTON:
				TermAddrFuncCheckBox(ShtSpd, &(uintptr_t){mdBaseAddr+0xC73EA}, 1, 2, ID_4_BUTTON);
				return 0;
			case ID_5_BUTTON:
				AddToKeepCycleCheckBox(txtBoxHealthHWND, KeepHealthCkBox, 0xEC, 0, 0);
				return 0;
			case ID_6_BUTTON:
				AddToKeepCycleCheckBox(txtBoxAmmoHWND, KeepAmmoCkBox, 0, AmmoOffsets, AmmoArrSize);
				return 0;
			case ID_8_BUTTON:
				wchar_t buf[256];
				DWORD val;
				
				GetWindowTextW(KeepCycleUpTime, buf, 256);
				swscanf(buf, L"%lu", &val);
				
				up_t = val;
				return 0;
			case ID_9_BUTTON:
				uintptr_t addrs[] = {mdBaseAddr+0xC8DD2, mdBaseAddr+0xC8DFB, mdBaseAddr+0xC8E03};
				TermAddrFuncCheckBox(KnockBackHWND, addrs, sizeof(addrs)/sizeof(addrs[0]), 5, ID_9_BUTTON);

				return 0;
			case ID_10_BUTTON:
				TermAddrFuncCheckBox(CamKnockbackHWND, &(uintptr_t){mdBaseAddr+0xC8E52}, 1, 5, ID_10_BUTTON);
				return 0;
			case ID_11_BUTTON:
				TermAddrFuncCheckBox(KnifeCdHWND, &(uintptr_t){mdBaseAddr+0xC624F}, 1, 2, ID_11_BUTTON);
				return 0;
			case ID_13_BUTTON:
				if (SendMessage(AimbotHWND, BM_GETCHECK, 0, 0) == BST_CHECKED)
				{
					aimbot = 1;
				} else if (SendMessage(AimbotHWND, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
				{
					aimbot = 0;
				}
				return 0;
			}
		}
		if (HIWORD(wParam) == EN_CHANGE) {
			switch (LOWORD(wParam))
			{
			case ID_12_BUTTON:
				wchar_t buf[256];
				int32_t val;
				
				GetWindowTextW(ChangeSpdTxtBoxHWND, buf, 256);
				swscanf(buf, L"%lu", &val);

				if (val == 0) return 0;

				uintptr_t addrs[] = {
					(mdBaseAddr+0xBFCB8)+3, (mdBaseAddr+0xBFC08)+3,
					(mdBaseAddr+0xBFC58)+3, (mdBaseAddr+0xBFD18)+3
				};

				int32_t valls[] = {val, -val, val, -val};
				ChangeAddrVal(hProcess, addrs, sizeof(addrs)/sizeof(addrs[0]), 1, valls);
				return 0;
			}
		}
		return 0;
	case WM_HSCROLL:
		if (LOWORD(wParam) == SB_THUMBTRACK) {
			WORD val = HIWORD(wParam);

			wchar_t buf[32];
			swprintf_s(buf, 32, L"%hu", val);

			SetWindowTextW(ChangeSpdTxtBoxHWND, buf);

			uintptr_t addrs[] = {
				(mdBaseAddr+0xBFCB8)+3, (mdBaseAddr+0xBFC08)+3,
				(mdBaseAddr+0xBFC58)+3, (mdBaseAddr+0xBFD18)+3
			};

			int32_t valls[] = {val, -val, val, -val};
			ChangeAddrVal(hProcess, addrs, sizeof(addrs)/sizeof(addrs[0]), 1, valls);
		}
		return 0;
	case WM_DESTROY:
        PostQuitMessage(0);

        CloseHandle(hProcess);
        CloseHandle(hThread);

        pthread_cancel(keepcycle_thread);

        return 0;
	}

	return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	const wchar_t CLASS_NAME[] = L"Assault Cube External Cheat Menu";

	WNDCLASSW wc = {0};

	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.hbrBackground = CreateSolidBrush(RGB(20, 20, 20));

	RegisterClassW(&wc);

	HWND hwnd = CreateWindowW(
		CLASS_NAME,
		L"AC External Cheat Menu",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, 700, 400,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (hwnd == NULL) {return 0;}

	ShowWindow(hwnd, nCmdShow);

	MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DeleteObject(wc.hbrBackground);
    return 0;
}

void AimCal(float* EnPos, int isDeltaPos) // if isDeltaPos - 1, it will use EnPos as deltaPos
{
	float deltaPos[3];

	for (int i=0; i<3; i++) {
		if (isDeltaPos == 1) {
			deltaPos[i] = EnPos[i];
			continue;
		}
		deltaPos[i] = EnPos[i]-Player.Pos[i];
	}

	float dist = sqrt(deltaPos[0] * deltaPos[0] + deltaPos[1] * deltaPos[1]);

	// yaw
	float x = atan2(deltaPos[1], deltaPos[0]) * (180.0f / PI) + 90.0f;
	// pitch
	float y = atan2(deltaPos[2], dist) * (180.0f / PI);

	WriteProcessMemory(hProcess, (LPVOID)(uintptr_t)(Player.baseAddr + 0x34), &x, sizeof(x), NULL); // x, yaw
	WriteProcessMemory(hProcess, (LPVOID)(uintptr_t)(Player.baseAddr + 0x38), &y, sizeof(y), NULL); // y, pitch
}

HHOOK g_hMouseHook = NULL;
int g_RBDown = 0;
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0) {
		if (wParam == WM_RBUTTONDOWN/* && aimbot == 1*/) {
			g_RBDown = 1;
		} else if (wParam == WM_RBUTTONUP) {
			g_RBDown = 0;
		}
	}
	return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}

void* RBDownSetUp()
{
	g_hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, NULL, 0);

	if (g_hMouseHook == NULL) {
		printf("Could not set up g_hMouseHook for mouse input listening, err: %lu\n", GetLastError());
		return (void*)1;
	}

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(g_hMouseHook);
}

void* RBDowndCycle()
{
	while (1) {
		if (aimbot == 0) continue;
		if (g_RBDown == 0) continue;

		PlayerReadMemory(&Player);
		EntityReadMemory(&NearestEnt);

		AimCal(NearestEnt.Pos, 1); // isDeltaPos - 1, because in EntityReadMemory we already got deltaPos
	}
}

void* KeepInfCycle()
{
	for (int i=0; i<10; i++) {
		_addroffsets.offset_arr[i] = 0;
		_addroffsets.value_arr[i] = 0;
	}

	uintptr_t baseAddr = mdBaseAddr + 0x17E0A8;
	
	uint32_t BAFP; // base addr from ptr
	ReadProcessMemory(hProcess, (LPCVOID)baseAddr, &BAFP, sizeof(BAFP), NULL);

	while (1) {
		for (int i=0; i<10; i++) {
			if (_addroffsets.offset_arr[i] != 0) {
				uintptr_t addr = BAFP + _addroffsets.offset_arr[i];
				int32_t val = _addroffsets.value_arr[i];

				WriteProcessMemory(hProcess, (LPVOID)addr, &val, sizeof(val), NULL);
			}
		}
		Sleep(up_t);
	}
}

void EnablePrivilege(wchar_t* name)
{
	HANDLE hToken;
	LUID luid;
	TOKEN_PRIVILEGES tp;

	// Opens handle to our current process
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken) == 1) {
		// Forthbringing locally unique id, needed to specify privilege name
		if (LookupPrivilegeValueW(NULL, name, &luid) == 1) {
			tp.PrivilegeCount = 1;
			tp.Privileges[0].Luid = luid;
			tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
			// Changes token of process, TOKEN_PRIVILEGES needed
			AdjustTokenPrivileges(hToken, 0, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
		}
		CloseHandle(hToken);
	}
}

int IsWithAdminRigths(void) {
    int isAdmin = FALSE;
    PSID administratorsGroup = NULL;

    // Represents the top-level authority of a security identifier. SECURITY_NT_AUTHORITY is 5th level
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    // Allocates and initializes a security id, with sunauthorities
    if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                 &administratorsGroup)) {
		// Checks if specified security id is enabled in access token.
		// If token handle (first arg) is NULL then uses the token of the calling thread
        CheckTokenMembership(NULL, administratorsGroup, &isAdmin);
        FreeSid(administratorsGroup);
    }
    return isAdmin;
}

int RunAsAdmin(void)
{
	wchar_t szPath[MAX_PATH];
	// Retrives full path to file
	GetModuleFileNameW(NULL, szPath, MAX_PATH);

	SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.lpVerb = L"runas";
    sei.lpFile = szPath;
    sei.hwnd = NULL;
    sei.nShow = SW_NORMAL;
    // Executes operation in a specified file
    if (!ShellExecuteExW(&sei)) {
    	printf("Could not open with admin rights, err: %lu\n", GetLastError());
    	return 0;
    }

    return 1;
}

void DisConsoleQuickEdit(void)
{
	// Get handle to the specified standard device(input, output, error)
	HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
	DWORD prevMode;

	GetConsoleMode(hInput, &prevMode);

	DWORD nMode = prevMode & ~ENABLE_QUICK_EDIT_MODE;
	nMode &= ~ENABLE_MOUSE_INPUT;

	SetConsoleMode(hInput, nMode);
}

int main(void)
{
	if (!IsWithAdminRigths()) {
		if (RunAsAdmin() == 1) {
			return 0;
		}
	}

	EnablePrivilege(L"SeDebugPrivilege");
	DisConsoleQuickEdit();

	printf("If cheat does not work, then type in cmd(with admin rights): taskkill /f /im ac_client.exe\n");
	printf("If cheat crashed, and cannot be opened again, then type in cmd: taskkill /f /im CheatACExternalMenu.exe\n");

	pid = get_pid_by_process_name("ac_client.exe");
	
	if (pid > 0)
    {
		hProcess = GetProcess(pid);
		hThread = get_hThread(pid);
		mdBaseAddr = get_module_base_addr("ac_client.exe", pid);
		
		if (hProcess == NULL) {
			printf("Could not open process, err: %lu\n", GetLastError());
		}
	} else {
		printf("Could not get pid, err: %lu\n", GetLastError());
	}

	pthread_t hRBDownThread; // hook
	pthread_t RBDownCycleThread; // held

	pthread_create(&keepcycle_thread, NULL, KeepInfCycle, NULL); // thread is canceled in WindowProc -> WM_DESTROY
	pthread_create(&hRBDownThread, NULL, RBDownSetUp, NULL);
	pthread_create(&RBDownCycleThread, NULL, RBDowndCycle, NULL);

	HINSTANCE hInstance = GetModuleHandleW(NULL);
    wWinMain(hInstance, NULL, GetCommandLineW(), SW_SHOWNORMAL);

    pthread_cancel(hRBDownThread);
    pthread_cancel(RBDownCycleThread);
	return 0;
}

HWND CreateButton(HWND hwnd, const wchar_t* txt, int px, int py, int w, int h, int ID)
{
	HWND hwndButton = CreateWindowW(
		L"BUTTON",
		txt,
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER | BS_DEFPUSHBUTTON,
		px,
		py,
		w,
		h,
		hwnd,
		(ID == 0 ? NULL : (HMENU)(uintptr_t)ID),
		(HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE),
		NULL
	);

	return hwndButton;
}

HWND CreateTxtBoxOrStcTxt(HWND hwnd, const wchar_t* className, const wchar_t* txt, int px, int py, int w, int h, int ID)
{
	HWND hwndEl = CreateWindowW(
		className,
		txt,
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER,
		px,
		py,
		w,
		h,
		hwnd,
		(ID == 0 ? NULL : (HMENU)(uintptr_t)ID),
		(HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE),
		NULL
	);

	return hwndEl;
}

HWND CreateCheckBox(HWND hwnd, const wchar_t* txt, int px, int py, int w, int h, int ID)
{
	HWND hwndButton = CreateWindowW(
		L"BUTTON",
		txt,
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER | BS_AUTOCHECKBOX,
		px,
		py,
		w,
		h,
		hwnd,
		(ID == 0 ? NULL : (HMENU)(uintptr_t)ID),
		(HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE),
		NULL
	);

	return hwndButton;
}

HWND CreateTrackbar(HWND hwnd, const wchar_t* txt, int px, int py, int w, int h, int ID)
{
	INITCOMMONCONTROLSEX icce;
	icce.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icce.dwICC = ICC_BAR_CLASSES;

	InitCommonControlsEx(&icce);

	HWND hwndTb = CreateWindowExW(
		0,
        L"msctls_trackbar32",
        txt,
        WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_TOP | TBS_FIXEDLENGTH,
        px, py,
        w, h,
        hwnd,
        (ID == 0 ? NULL : (HMENU)(uintptr_t)ID),
        (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE),
        NULL
	);

	return hwndTb;
}