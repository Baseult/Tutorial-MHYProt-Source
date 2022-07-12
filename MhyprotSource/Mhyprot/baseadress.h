#pragma once
#include <cstdint>
uint64_t get_process_base(int pid);
uint64_t get_process_id(const char* process_name);
uint64_t get_process_id();
bool is_valid(uint64_t adress);

bool read_raw(int pid, uint64_t address, void* buffer, size_t size);
bool write_raw(int pid, uint64_t address, void* buffer, size_t size);

template <typename T>
inline bool read(uint64_t address, T& value)
{
	return read_raw(get_process_id(), address, &value, sizeof(T));
}

template <typename T>
inline bool write(uint64_t address, const T& value)
{
	if (is_valid(address))
	{
		return write_raw(get_process_id(), address, (void*)&value, sizeof(T));
	}
	return false;
}

template <typename T>
inline bool read_array(uint64_t address, T* array, size_t len)
{
	return read_raw(get_process_id(), address, array, sizeof(T) * len);
}

template <typename T>
inline T read(uint64_t address)
{
	T buffer{};
	if (is_valid(address))
	{
		read(address, buffer);
	}
	return buffer;
}
