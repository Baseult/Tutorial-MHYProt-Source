#pragma once
#include <cstdint>

#include "Vector3.h"

class initialize_offsets
{
public:
	static bool init_offsets();
	static Vector3 local_player_position();
	static bool is_valid(const uint64_t adress);
};


extern uint64_t process_base;
extern uint64_t m_world;
extern uint64_t world;
extern uint64_t camera;
extern uint64_t current_object;
extern uint64_t first_object;
extern uint64_t camera_matrix;
extern uint64_t gamex;
extern uint64_t graphics;

extern uint64_t midentlist;
extern uint64_t entitylist;

extern uint64_t  m_entitylist;
extern uint64_t m_midentlist;
extern uint64_t  m_entitylistcount;
extern uint64_t m_midentlistcount;

#define SCREEN_WIDTH	2560 //Enter your Screen Width
#define SCREEN_HEIGHT	1440 //Enter your Screen Height
