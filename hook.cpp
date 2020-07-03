#include "mem.h"
#include "hook.h"
#include <iostream>


// This is where all of the magic happens to insert the detour to our code
// There are many different ways to actually cause the jmp, the implementation
// below just uses a relative jmp (5 bytes), and only works for 32b.
bool Detour32(BYTE* src, BYTE* dst, const uintptr_t len)
{
	// Min size of jmps is 5 bytes on 32b
	if (len < sizeOfJmpPatch) {
		return false;
	}

	//  There are two major scenarios that need to be covered.
	// 1. Easily replace 5 bytes without any extra stolen bytes:
	//  1005519e 66 90           XCHG       EAX, EAX
	//	100551a0 55              PUSH       EBP
	//	100551a1 8b ec           MOV        EBP, ESP
	//	100551a3 83 e4 f8        AND        ESP, 0xfffffff8
	// We need to steal 5 bytes for the unconditional jump that's being added and nothing tricky here.
	//
	// 2. When 5 bytes isn't enough and more need to be stolen like below with 6 bytes
	//	100551a0 55              PUSH       EBP
	//	100551a1 8b ec           MOV        EBP, ESP
	//	100551a3 83 e4 f8        AND        ESP, 0xfffffff8

	// Change the permissions of the function we want to detour
	DWORD curProtection;
	VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &curProtection);

	// The src is the original function we are hooking
	// The dst is the address of the gateway which has the stolen bytes as well as the
	// jump back to the original function
	uintptr_t relAddr = dst - src - sizeOfJmpPatch;

	printf("Patching addr: 0x%Ix to point to 0x%Ix\n", (uintptr_t)src, (uintptr_t)dst);

	// Memset the entire length of memory before writing the jmp.
	// In the case where >5 bytes need to be stolen, there'll be nothing needed to be done
	// afterwards as it'll already be nop'd
	memset(src, 0x90, len);

	// E9 is the unconditional jmp instruction
	*(src) = 0xE9;

	// Write in the rel addr to jump to
	*(uintptr_t*)(src + 1) = relAddr;

	// Set the permissions back from the saved curProtection
	VirtualProtect(src, len, curProtection, &curProtection);

	return true;
}

// This function creates roughly the following layout in the gateway memory:
//  +-------------------+
//	| gateway:          |
//	|   nop             |--|
//	|   push ebp        |  |--> Stolen Bytes (this will vary depending on how many bytes stolen)
//	|   move ebp, esp   |--|
//	|   jmp <relAddr>   |--> Addr of the function owned by the injected DLL
//	+-------------------+
// The gateway allows us to get back to the orignal function after the relative jump.
BYTE* TrampHook32(BYTE* src, BYTE* dst, const uintptr_t len)
{
	// Min size of jmps is 5 bytes on 32b
	if (len < sizeOfJmpPatch)
	{
		return 0;
	}

	// Create the gateway with execute permissions and read/write for our function
	BYTE* gateway = (BYTE*)VirtualAlloc(0, len, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	// Write the stolen bytes to the gateway, these come from the original function we are hooking
	memcpy_s(gateway, len, src, len);

	// Get the gateway addr back to the destination address
	uintptr_t gatwayRelAddr = src - gateway - sizeOfJmpPatch;

	// Add the jmp to to our gateway
	*(gateway + len) = 0xE9;

	*(uintptr_t*)(gateway + len + 1) = gatwayRelAddr;

	// Preform the detour
	Detour32(src, dst, len);

	// Returning the gateway here helps prevent hook recursion
	return gateway;
}

// Similar to the TrampHook32, except in reverse. Restores the original bytes.
bool UnTrampHook32(BYTE* src, BYTE* dst, const uintptr_t len)
{
	// Min size of jmps is 5 bytes on 32b
	if (len < sizeOfJmpPatch)
	{
		return 0;
	}

	// Change the permissions of the function we want to undetour, aka the dst
	DWORD curProtection;
	VirtualProtect(dst, len, PAGE_EXECUTE_READWRITE, &curProtection);

	// Write the stolen bytes from the gateway back to the orignal function, these come from the original function we are hooking
	memcpy_s(dst, len, src, len);

	// Set the permissions back from the saved curProtection
	VirtualProtect(dst, len, curProtection, &curProtection);

	// Returning the gateway here helps prevent hook recursion
	return true;
}

// Constructor for the hook.
Hook::Hook(BYTE* src, BYTE* dst, BYTE* PtrToGatewayPtr, uintptr_t len)
{
	this->m_src = src;
	this->m_dst = dst;
	this->m_len = len;
	this->m_ptrToGatewayFnPtr = PtrToGatewayPtr;
}

Hook::Hook(const char* exportName, const char* modName, BYTE* dst, BYTE* PtrToGatewayPtr, uintptr_t len)
{
	// Get a handle to the module in question
	HMODULE hModule = GetModuleHandleA(modName);

	// Get the address from the module with the name it was exported with
	this->m_src = (BYTE*)GetProcAddress(hModule, exportName);
	this->m_dst = dst;
	this->m_len = len;
	this->m_ptrToGatewayFnPtr = PtrToGatewayPtr;
}

void Hook::Enable()
{
	if (m_len > 10)
	{
		printf("Hook not enabled. Too many stolen bytes.\n");
		return;
	}

	// Save off the original bytes so they can be used later when disabling the hook
	memcpy(m_originalBytes, m_src, m_len);

	*(uintptr_t*)m_ptrToGatewayFnPtr = (uintptr_t)TrampHook32(m_src, m_dst, m_len);
	m_EnabledStatus = true;
}

void Hook::Disable()
{
	UnTrampHook32(m_originalBytes, m_src, m_len);
	m_EnabledStatus = false;
}

void Hook::Toggle()
{
	if (m_EnabledStatus == true)
	{
		Disable();
	}
	else
	{
		Enable();
	}
}

// Not used for PA3
bool GetPEHeader(const char* modName, PIMAGE_DOS_HEADER pDosHeader, PIMAGE_NT_HEADERS pNtHeader)
{
	pDosHeader = (PIMAGE_DOS_HEADER)GetModuleHandleA(modName);
	pNtHeader = (PIMAGE_NT_HEADERS)PtrFromRva(pDosHeader, pDosHeader->e_lfanew);

	// Make sure we have valid data
	if (pNtHeader->Signature != IMAGE_NT_SIGNATURE)
		return FALSE;

	// Grab a pointer to the import data directory
	PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)PtrFromRva(pDosHeader, pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	if ((pDosHeader) && (pImportDescriptor)) {
		// This loop will parse the entries inside the import descriptor
		for (UINT uIndex = 0; pImportDescriptor[uIndex].Characteristics != 0; uIndex++)
		{
			char* szDllName = (char*)PtrFromRva(pDosHeader, pImportDescriptor[uIndex].Name);
			printf("%s\n", szDllName);
		}
	}
	return false;
}