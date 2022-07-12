#pragma once
#include <set>
#include "Overlay.h"

struct EspInfo
{
	Vector3 Position;
	Vector3 HeadPosition;
	float Health;
	int entity;
	bool dead;
	bool ismyteam;
	std::string Name;
};

namespace safe_esp
{
	extern uint64_t mytarget;
	extern uint64_t skiptarget;
	extern std::set<uintptr_t> skiplist;
	extern float distancecheck;
}

class Esp
{
public:
	static void esp_draw();
	static void esp_loop();
};
