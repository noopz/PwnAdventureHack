#pragma once
#include <vector>
#include <windows.h>
#include <TlHelp32.h>

// Not used for PA3

// Based on the code from https://guidedhacking.com

DWORD GetProcId(const wchar_t* procName);

uintptr_t GetModuleBaseAddress(DWORD procId, const wchar_t* modName);

uintptr_t FindDMAAddy(HANDLE hProc, uintptr_t ptr, std::vector<unsigned int> offsets);