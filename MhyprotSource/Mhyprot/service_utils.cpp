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

#include "service_utils.hpp"
#include "../skCrypter.h"

//
// open service control manager to operate services
//
SC_HANDLE service_utils::open_sc_manager()
{
	return OpenSCManager(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
}

//
// create a new service
// sc create myservice binPath="" type=kernel
//
SC_HANDLE service_utils::create_service(const std::string_view driver_path)
{
	const SC_HANDLE sc_manager_handle = open_sc_manager();

	CHECK_SC_MANAGER_HANDLE(sc_manager_handle, (SC_HANDLE)INVALID_HANDLE_VALUE);

	const SC_HANDLE mhyprot_service_handle = CreateService(
		sc_manager_handle,
		MHYPROT_SERVICE_NAME,
		MHYPROT_DISPLAY_NAME,
		SERVICE_START | SERVICE_STOP | DELETE,
		SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE,
		driver_path.data(), nullptr, nullptr, nullptr, nullptr, nullptr
	);

	if (!CHECK_HANDLE(mhyprot_service_handle))
	{
		if (const auto last_error = GetLastError(); last_error == ERROR_SERVICE_EXISTS)
		{
			logger::log2(skCrypt("[+] the service already exists, open handle\n"));

			return OpenService(
				sc_manager_handle,
				MHYPROT_SERVICE_NAME,
				SERVICE_START | SERVICE_STOP | DELETE
			);
		}

		logger::log2(skCrypt("[!] failed to create %s service. (0x%lX)\n"), MHYPROT_SERVICE_NAME, GetLastError());
		CloseServiceHandle(sc_manager_handle);
		return static_cast<SC_HANDLE>((INVALID_HANDLE_VALUE));
	}

	CloseServiceHandle(sc_manager_handle);

	return mhyprot_service_handle;
}

//
// delete the service
// sc delete myservice
//
bool service_utils::delete_service(const SC_HANDLE service_handle, const bool close_on_fail, const bool close_on_success)
{
	const SC_HANDLE sc_manager_handle = open_sc_manager();

	CHECK_SC_MANAGER_HANDLE(sc_manager_handle, false);

	if (!DeleteService(service_handle))
	{
		if (const auto last_error = GetLastError(); last_error == ERROR_SERVICE_MARKED_FOR_DELETE)
		{
			CloseServiceHandle(sc_manager_handle);
			return true;
		}

		logger::log2(skCrypt("[!] failed to delete the service. (0x%lX)\n"), GetLastError());
		CloseServiceHandle(sc_manager_handle);
		if (close_on_fail) CloseServiceHandle(service_handle);
		return false;
	}

	CloseServiceHandle(sc_manager_handle);
	if (close_on_success) CloseServiceHandle(service_handle);

	return true;
}

//
// start the service
// sc start myservice
//
bool service_utils::start_service(const SC_HANDLE service_handle)
{
	return StartService(service_handle, 0, nullptr);
}

//
// stop the service
// sc stop myservice
//
bool service_utils::stop_service(const SC_HANDLE service_handle)
{
	const SC_HANDLE sc_manager_handle = open_sc_manager();

	CHECK_SC_MANAGER_HANDLE(sc_manager_handle, false);

	SERVICE_STATUS service_status;

	if (!ControlService(service_handle, SERVICE_CONTROL_STOP, &service_status))
	{
		logger::log2(skCrypt("[!] failed to stop the service. (0x%lX)\n"), GetLastError());
		CloseServiceHandle(sc_manager_handle);
		return false;
	}

	CloseServiceHandle(sc_manager_handle);

	return true;
}
