// simpleinjector.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include "proc.h"

// This simple injector is one of the many different ways
// to inject a custom DLL into a process. This way is trival
// and causes Windows to flag it as potential malware. If that occurs
// then look into excepting this while doing development.
// The general flow is three steps:
// 1. Allocate space for the DLL-to-be-inject's name
// 2. Write the absoulte path + name to the DLL to the memory from (1)
// 3. Spawn a thread calling LoadLibrary at the location from (2)
int main()
{
	// Since this is a specific hack for PA3 hardcoding values.
	const char* dllPath = "C:\\git\\pa3hack\\Debug\\pa3hack.dll";
	const wchar_t* procName = L"PwnAdventure3-Win32-Shipping.exe";
	DWORD procId = 0;

	while (!procId)
	{
		procId = GetProcId(procName);
		Sleep(30);
	}

	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, 0, procId);

	if ((hProc != nullptr) && (hProc != INVALID_HANDLE_VALUE))
	{
		// This just loads the path name to the DLL so don't need page execution permissions
		// Gives a location to memory in the processes' space.
		void* loc = VirtualAllocEx(hProc, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

		// Write the dll's path into the applications memory assuming its valid
		if (loc != 0)
		{
			WriteProcessMemory(hProc, loc, dllPath, strlen(dllPath) + 1, 0);
		}

		// Now create a thread that calls Loadlibray at the location we've specified (aka the path to the dll)
		HANDLE hThread = CreateRemoteThread(hProc, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, loc, 0, 0);

		if (hThread != nullptr)
		{
			CloseHandle(hThread);
		}

	}

	if (hProc)
	{
		CloseHandle(hProc);
	}

	return 0;
}