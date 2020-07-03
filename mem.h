#pragma once

#include <windows.h>
#include <vector>
//https://guidedhacking.com

namespace mem
{
	// Internal versions - most notable difference no process handle
	void Nop(BYTE* dst, unsigned int size);
	void Patch(BYTE* dst, BYTE* src, unsigned int size);

	// External versions
	void PatchEx(BYTE* dst, BYTE* src, unsigned int size, HANDLE hProcess);
	void NopEx(BYTE* dst, unsigned int size, HANDLE hProcess);

	// No process handle here either
	uintptr_t FindDMAAddy(uintptr_t ptr, std::vector<unsigned int> offsets);

}