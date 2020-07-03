#pragma once
#include <Windows.h>
#include "mem.h"

// The code here is based off of the Guided Hacking tutorials

static uint32_t sizeOfJmpPatch = 5;

// This macro will help with calculating addresses
#define PtrFromRva( base, rva ) ( ( ( PBYTE ) base ) + rva )
bool GetPEHeader(const char* modName, PIMAGE_DOS_HEADER pDosHeader, PIMAGE_NT_HEADERS pNtHeaders);

bool Detour32(BYTE* src, BYTE* dst, const uintptr_t len);
bool UnTrampHook32(BYTE* src, BYTE* dst, const uintptr_t len);
BYTE* TrampHook32(BYTE* src, BYTE* dst, const uintptr_t len);

class Hook
{
public:
	Hook(BYTE* src, BYTE* dst, BYTE* PtrToGatewayPtr, uintptr_t len);
	Hook(const char* exportName, const char* modName, BYTE* dst, BYTE* PtrToGatewayPtr, uintptr_t len);

	void Enable();
	void Disable();
	void Toggle();

private:
	bool m_EnabledStatus{ false };
	BYTE* m_src{ nullptr };
	BYTE* m_dst{ nullptr };
	BYTE* m_ptrToGatewayFnPtr{ nullptr };
	uintptr_t m_len{ 0 };
	BYTE m_originalBytes[10]{ 0 };
};