
#include "Offsets.h"
#include "skCrypter.h"
#include "Vector3.h"
#include "Mhyprot/baseadress.h"

uint64_t m_entitylist = 0x00;
uint64_t m_entitylistcount = 0x00;
uint64_t m_world = 0x00;

uint64_t process_base = 0x0;
uint64_t world = 0x0;
uint64_t camera = 0x0;
uint64_t entitylist = 0x0;

bool initialize_offsets::init_offsets()
{

	auto process_name = skCrypt("game.exe");
	const auto process_id = get_process_id(process_name);
	process_base = get_process_base(process_id);

	world = read<uint64_t>(process_base + m_world);
	camera = read<uint64_t>(world + 0x00);
	entitylist = read<uint64_t>(world +  m_entitylist);

	return true;

}

Vector3 initialize_offsets::local_player_position()
{
	//Yes I could just read it as Vector3 once instead but doing it this way for novices to understand
	const auto myposx = read<float>(world + 0x00);	//Get your X position
	const auto myposy = read<float>(world + 0x00);	//Get your Y position
	const auto myposz = read<float>(world + 0x00);	//Get your Z position
	return {myposx, myposy, myposz};
}

bool initialize_offsets::is_valid(const uint64_t adress)
{
	if (adress <= 0x400000 || adress == 0xCCCCCCCCCCCCCCCC || reinterpret_cast<void*>(adress) == nullptr || adress >
		0x7FFFFFFFFFFFFFFF)
	{
		return false;
	}

	return true;
}

//bool Example_Patternscan() {
//
//	auto TestPatternscan = PatternScan<uint32_t>(process_base, skCrypt("48 8B 0D ? ? ? ? 48 85 C9 0F 84 ? ? ? ? 48"), 3);
//}
//
//template<class T>
//T PatternScan(uintptr_t moduleAdress, const char* signature, int offset) {
//	int instructionLength = offset + sizeof(T);
//	IMAGE_DOS_HEADER dos_header;
//	IMAGE_NT_HEADERS64 nt_headers;
//	read(process_base, dos_header);
//	read(process_base + dos_header.e_lfanew, nt_headers);
//
//	const size_t target_len = nt_headers.OptionalHeader.SizeOfImage;
//
//	static auto patternToByte = [](const char* pattern)
//	{
//		auto       bytes = std::vector<int>{};
//		const auto start = const_cast<char*>(pattern);
//		const auto end = const_cast<char*>(pattern) + strlen(pattern);
//
//		for (auto current = start; current < end; ++current)
//		{
//			if (*current == '?')
//			{
//				++current;
//				bytes.push_back(-1);
//			}
//			else { bytes.push_back(strtoul(current, &current, 16)); }
//		}
//		return bytes;
//	};
//
//	auto       patternBytes = patternToByte(signature);
//	const auto s = patternBytes.size();
//	const auto d = patternBytes.data();
//
//	auto target = std::unique_ptr<uint8_t[]>(new uint8_t[target_len]);
//	if (read_array(process_base, target.get(), target_len)) {
//		for (auto i = 0ul; i < nt_headers.OptionalHeader.SizeOfImage - s; ++i)
//		{
//			bool found = true;
//			for (auto j = 0ul; j < s; ++j)
//			{
//				if (target[static_cast<size_t>(i) + j] != d[j] && d[j] != -1)
//				{
//					found = false;
//					break;
//				}
//			}
//			if (found) {
//				return read<T>(moduleAdress + i + offset) + i + instructionLength;
//			}
//		}
//	}
//
//	return NULL;
//}
