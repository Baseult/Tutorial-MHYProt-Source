#pragma once
#include "Vector3.h"

class Noclip
{
public:
	static void on_fast_update();
	static void Initialize();
	static void on_update();
	static Vector3 current_position;

private:
	static void calculate_new_position();
};
