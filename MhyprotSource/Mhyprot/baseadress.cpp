
#include "baseadress.h"
#include <stdio.h>
#include "mhyprot.hpp"
#include "readbase.hpp"

uint64_t process_id;
uint64_t systembase = 0;
uint64_t ep_image_file_name = 0;
uint64_t ep_unique_process_id = 0;
uint64_t ep_section_base_address = 0;
uint64_t ep_active_process_links = 0;

bool read_virtual(const uint64_t address, uint8_t* buffer, const int size)
{
	const auto paddress = address;
	return mhyprot::driver_impl::read_kernel_memory(paddress, buffer, size);
}

uint64_t sf_get_e_process(const int pid)
{
	const auto handle_info = QueryInfo<SYSTEM_HANDLE_INFORMATION>(SystemHandleInformation);
	if (!handle_info)
		return 0;

	for (size_t i = 0; i < handle_info->HandleCount; i++)
		if (pid == handle_info->Handles[i].ProcessId && 7 == handle_info->Handles[i].ObjectTypeNumber)
		{
			//logger::log2("[+] Got Handle is 0x%llX\n", reinterpret_cast<size_t>(handle_info->Handles[i].Object));
			return reinterpret_cast<size_t>(handle_info->Handles[i].Object);
		}
	return 0;
}

uint64_t get_e_process(const int pid)
{
	_LIST_ENTRY active_process_links;

	read_virtual(sf_get_e_process(4) + ep_active_process_links, reinterpret_cast<uint8_t*>(&active_process_links), sizeof(active_process_links));

	while (true)
	{
		uint64_t next_pid = 0;
		char buffer[0xFFFF];
		const uint64_t next_link = reinterpret_cast<uint64_t>(active_process_links.Flink);
		const uint64_t next = next_link - ep_active_process_links;
		read_virtual(next + ep_unique_process_id, reinterpret_cast<uint8_t*>(&next_pid), sizeof(next_pid));
		read_virtual(next + ep_image_file_name, reinterpret_cast<uint8_t*>(&buffer), sizeof(buffer));
		read_virtual(next + ep_active_process_links, reinterpret_cast<uint8_t*>(&active_process_links), sizeof(active_process_links));
		if (GetAsyncKeyState(VK_MENU))
		{
			mhyprot::unload();
		}

		if (next_pid == pid)
		{
			return next;
		}
		if (next_pid == 4 || next_pid == 0)
			break;
	}
	return 0;
}

uint64_t get_kernel_module_address(const std::string& module_name)
{
	void* buffer = nullptr;
	DWORD buffer_size = 0;

	NTSTATUS status = NtQuerySystemInformation(static_cast<SYSTEM_INFORMATION_CLASS>(SystemModuleInformation), buffer,
	                                           buffer_size, &buffer_size);

	while (status == STATUS_INFO_LENGTH_MISMATCH)
	{
		VirtualFree(buffer, 0, MEM_RELEASE);

		buffer = VirtualAlloc(nullptr, buffer_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		status = NtQuerySystemInformation(static_cast<SYSTEM_INFORMATION_CLASS>(SystemModuleInformation), buffer,
		                                  buffer_size, &buffer_size);
	}

	if (!NT_SUCCESS(status))
	{
		VirtualFree(buffer, 0, MEM_RELEASE);
		return 0;
	}

	const auto modules = static_cast<PRTL_PROCESS_MODULES>(buffer);

	for (auto i = 0u; i < modules->NumberOfModules; ++i)
	{
		const auto current_module_name = std::string(
			reinterpret_cast<char*>(modules->Modules[i].FullPathName) + modules->Modules[i].OffsetToFileName);

		if (!_stricmp(current_module_name.c_str(), module_name.c_str()))
		{
			const auto result = reinterpret_cast<uint64_t>(modules->Modules[i].ImageBase);

			VirtualFree(buffer, 0, MEM_RELEASE);
			return result;
		}
	}

	VirtualFree(buffer, 0, MEM_RELEASE);
	return 0;
}

void fix_offsets()
{
	using namespace std;

	NTSTATUS (WINAPI * rtl_get_version)(LPOSVERSIONINFOEXW);
	OSVERSIONINFOEXW os_info;

	*reinterpret_cast<FARPROC*>(&rtl_get_version) = GetProcAddress(GetModuleHandleA("ntdll"),
	                                                               "RtlGetVersion");

	DWORD build = 0;

	if (nullptr != rtl_get_version)
	{
		os_info.dwOSVersionInfoSize = sizeof(os_info);
		rtl_get_version(&os_info);
		build = os_info.dwBuildNumber;
	}

	//https://www.vergiliusproject.com/kernels/x64/Windows%2010%20|%202016/2009%2020H2%20(October%202020%20Update)/_EPROCESS
	switch (build) // some offsets might be wrong, check it yourself it if does not work
	{
	case 22000: //WIN11
		ep_image_file_name = 0x5a8;
		ep_unique_process_id = 0x440;
		ep_section_base_address = 0x520;
		ep_active_process_links = 0x448;
		break;
	case 19044: //WIN10_21H2
		ep_image_file_name = 0x5a8;
		ep_unique_process_id = 0x440;
		ep_section_base_address = 0x520;
		ep_active_process_links = 0x448;
		break;
	case 19043: //WIN10_21H1
		ep_image_file_name = 0x5a8;
		ep_unique_process_id = 0x440;
		ep_section_base_address = 0x520;
		ep_active_process_links = 0x448;
		break;
	case 19042: //WIN10_20H2
		ep_image_file_name = 0x5a8;
		ep_unique_process_id = 0x440;
		ep_section_base_address = 0x520;
		ep_active_process_links = 0x448;
		break;
	case 19041: //WIN10_20H1
		ep_image_file_name = 0x5a8;
		ep_unique_process_id = 0x440;
		ep_section_base_address = 0x520;
		ep_active_process_links = 0x448;
		break;
	case 18363: //WIN10_19H2
		ep_image_file_name = 0x450;
		ep_unique_process_id = 0x2e8;
		ep_section_base_address = 0x3c8;
		ep_active_process_links = 0x2f0;
		break;
	case 18362: //WIN10_19H1
		ep_image_file_name = 0x450;
		ep_unique_process_id = 0x2e8;
		ep_section_base_address = 0x3c8;
		ep_active_process_links = 0x2f0;
		break;
	case 17763: //WIN10_RS5
		ep_image_file_name = 0x450;
		ep_unique_process_id = 0x2e0;
		ep_section_base_address = 0x3c0;
		ep_active_process_links = 0x2e8;
		break;
	case 17134: //WIN10_RS4
		ep_image_file_name = 0x450;
		ep_unique_process_id = 0x2e0;
		ep_section_base_address = 0x3c0;
		ep_active_process_links = 0x2e8;
		break;
	case 16299: //WIN10_RS3
		ep_image_file_name = 0x450;
		ep_unique_process_id = 0x2e0;
		ep_section_base_address = 0x3c0;
		ep_active_process_links = 0x2e8;
		break;
	case 15063: //WIN10_RS2
		ep_image_file_name = 0x450;
		ep_unique_process_id = 0x2e0;
		ep_section_base_address = 0x3c0;
		ep_active_process_links = 0x2e8;
		break;
	case 14393: //WIN10_RS1
		ep_image_file_name = 0x450;
		ep_unique_process_id = 0x2e8;
		ep_section_base_address = 0x3c0;
		ep_active_process_links = 0x2f0;
		break;
	case 10586: //WIN10_TH2
		ep_image_file_name = 0x450;
		ep_unique_process_id = 0x2e8;
		ep_section_base_address = 0x3c0;
		ep_active_process_links = 0x2f0;
		break;
	default:
		printf(
			"[!] No Support for %d,\n check https://www.vergiliusproject.com/kernels/x64/Windows%2011/Insider%20Preview%20(Jun%202021)/_EPROCESS to update the code\n",
			build);
		exit(0);
		break;
	}
	printf("[+] Found offsets for %d!\n", build);
}

uint64_t get_process_id()
{
	return process_id;
}

uint64_t get_process_base(const int pid)
{
	fix_offsets();

	if (systembase == 0)
	{
		systembase = get_kernel_module_address("ntoskrnl.exe");
	}
	uint64_t base = 0;
	logger::log2(skCrypt("[+] Got System Base is 0x%llX\n"), systembase);
	read_virtual(get_e_process(pid) + ep_section_base_address, (uint8_t*)&base, sizeof(base));
	logger::log2(skCrypt("[+] Got Process Base is 0x%llX\n"), base);
	process_id = pid;
	return base;
}

uint64_t get_process_id(const char* process_name)
{
	UINT pid = 0;
	const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 process;
	ZeroMemory(&process, sizeof(process));
	process.dwSize = sizeof(process);
	if (Process32First(snapshot, &process))
	{
		do
		{
			if (std::strcmp(process.szExeFile, process_name) == 0)
			{
				//printf("PID::: %d : [%s] , %s\n\n", process.th32ProcessID, process.szExeFile, process_name);
				pid = process.th32ProcessID;
				break;
			}
		}
		while (Process32Next(snapshot, &process));
	}
	CloseHandle(snapshot);
	return pid;
}

bool read_raw(const int pid, const uint64_t address, void* buffer, const size_t size)
{
	return mhyprot::driver_impl::read_user_memory(pid, address, buffer, size);
}

bool write_raw(const int pid, const uint64_t address, void* buffer, const size_t size)
{
	return mhyprot::driver_impl::write_user_memory(pid, address, buffer, size);
}

bool is_valid(const uint64_t adress)
{
	if (adress <= 0x400000 || adress == 0xCCCCCCCCCCCCCCCC || reinterpret_cast<void*>(adress) == nullptr || adress >
		0x7FFFFFFFFFFFFFFF)
	{
		return false;
	}

	return true;
}
