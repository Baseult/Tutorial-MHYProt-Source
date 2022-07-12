/*
 * MIT License
 *
 * Copyright (c) 2020 Kento Oki
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "win_utils.hpp"
#include "../skCrypter.h"

//
// find the process id by specific name using ToolHelp32Snapshot
//
uint32_t win_utils::find_process_id(const std::string_view process_name)
{
	PROCESSENTRY32 processentry = {};

	const unique_handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0), &CloseHandle);

	if (!CHECK_HANDLE(snapshot.get()))
	{
		logger::log2(skCrypt("[!] Failed to create ToolHelp32Snapshot [0x%lX]\n"), GetLastError());
		return 0;
	}

	processentry.dwSize = sizeof(MODULEENTRY32);

	while (Process32Next(snapshot.get(), &processentry) == TRUE)
	{
		if (process_name.compare(processentry.szExeFile) == 0)
		{
			return processentry.th32ProcessID;
		}
	}

	return 0;
}

//
// find the base address of process by the pid using ToolHelp32Snapshot
//
uint64_t win_utils::find_base_address(const uint32_t process_id)
{
	MODULEENTRY32 module_entry = {};

	const unique_handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process_id),
	                             &CloseHandle);

	if (!CHECK_HANDLE(snapshot.get()))
	{
		printf(skCrypt("[!] Failed to create ToolHelp32Snapshot [0x%lX]\n"), GetLastError());
		return 0;
	}

	module_entry.dwSize = sizeof(module_entry);

	Module32First(snapshot.get(), &module_entry);

	return reinterpret_cast<uint64_t>(module_entry.modBaseAddr);
}

//
// lookup base address of specific module that loaded in the system
// by NtQuerySystemInformation api
//
uint64_t win_utils::obtain_sysmodule_address(
	const std::string_view target_module_name,
	bool debug_prints
)
{
	const HMODULE module_handle = GetModuleHandle(TEXT(skCrypt("ntdll.dll")));

			if (!CHECK_HANDLE(module_handle))
			{
				logger::log2(skCrypt("[!] failed to obtain ntdll.dll handle. (0x%lX)\n"), module_handle);
				return 0;
			}

			const auto nt_query_system_information =
				reinterpret_cast<pNtQuerySystemInformation>(GetProcAddress(module_handle, "NtQuerySystemInformation"));

			if (!nt_query_system_information)
			{
				logger::log2(skCrypt("[!] failed to locate NtQuerySystemInformation. (0x%lX)\n"), GetLastError());
				return 0;
			}

			NTSTATUS status;
			PVOID buffer;
			ULONG alloc_size = 0x10000;
			ULONG needed_size;

			do
			{
				buffer = calloc(1, alloc_size);

				if (!buffer)
				{
					logger::log2(skCrypt("[!] failed to allocate buffer for query (0). (0x%lX)\n"), GetLastError());
					return 0;
				}

				status = nt_query_system_information(
					SystemModuleInformation,
					buffer,
					alloc_size,
					&needed_size
				);

				if (!NT_SUCCESS(status) && status != STATUS_INFO_LENGTH_MISMATCH)
				{
					logger::log2(skCrypt("[!] failed to query system module information. NTSTATUS: 0x%llX\n"), status);
					free(buffer);
					return 0;
				}

				if (status == STATUS_INFO_LENGTH_MISMATCH)
				{
					free(buffer);
					buffer = nullptr;
					alloc_size *= 2;
				}
			}
			while (status == STATUS_INFO_LENGTH_MISMATCH);

			if (!buffer)
			{
				logger::log2(skCrypt("[!] failed to allocate buffer for query (1). (0x%lX)\n"), GetLastError());
				return 0;
			}

			const auto module_information = static_cast<PSYSTEM_MODULE_INFORMATION>(buffer);

			logger::log2(skCrypt("[>] looking for %s in sysmodules...\n"), target_module_name.data());

			for (ULONG i = 0; i < module_information->Count; i++)
			{
				SYSTEM_MODULE_INFORMATION_ENTRY module_entry = module_information->Module[i];
				auto module_address = reinterpret_cast<ULONG_PTR>(module_entry.DllBase);

				if (module_address < MIN_ADDRESS)
				{
					continue;
				}

				PCHAR module_name = module_entry.ImageName + module_entry.ModuleNameOffset;

				if (debug_prints)
				{
					logger::log2(skCrypt("[+] sysmodule: %025s @ 0x%llX\n"), module_name, module_address);
				}

				if (target_module_name.compare(module_name) == 0 ||
					std::string(module_name).find(skCrypt("mhyprot")) != std::string::npos)
				{
					logger::log2(skCrypt("[<] found\n"));
					return module_address;
				}
			}

			free(buffer);
			return 0;
		}
