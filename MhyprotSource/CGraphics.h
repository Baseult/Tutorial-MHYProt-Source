#pragma once
#include "Vector3.h"

class c_graphics_instance
{
public:
	static Vector3 world_to_screen(const Vector3 position);
	static bool initializeit();
private:
};



