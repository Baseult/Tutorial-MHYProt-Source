#include <iostream>

#include "Noclip.h"
#include "Inputs.h"
#include "Offsets.h"
#include "settings.h"
#include "skCrypter.h"
#include "../MhyprotSource/Mhyprot/baseadress.h"

# define PI           3.14159265358979323846f  /* pi */

bool do_init = true;
bool noclip_active = false;

uint64_t local_player = 0;
uint64_t m_player_base = 0;
Vector3 m_view_angles = Vector3(0, 0, 0);
Vector3 Noclip::current_position = Vector3(0, 0, 0);
Vector3 m_position = Vector3(0, 0, 0);
uint64_t process_basey;

//Selfcoded trash method of doing noclip. someon please recode this in a better way

int get_foward_factor()	//get w and s movement
{
	int ws_factor = 0;
	if (GetAsyncKeyState(0x57) & 0x8000) ++ws_factor;
	if (GetAsyncKeyState(0x53) & 0x8000) --ws_factor;
	return ws_factor;
}

int get_side_factor() //get a and d movement
{
	int ad_factor = 0;
	if (GetAsyncKeyState(0x41) & 0x8000) ++ad_factor;
	if (GetAsyncKeyState(0x44) & 0x8000) --ad_factor;
	return ad_factor;
}

float get_up_factor() //get up and down movement (Capslock and Shift)
{
	float ad_factor = 0;

		if (GetAsyncKeyState(VK_CAPITAL) & 0x8000) ad_factor += 1;
		else if (GetAsyncKeyState(VK_SHIFT) & 0x8000) ad_factor -= 1;
		else ad_factor = 0;

	return ad_factor;
}

Vector3 Safe1;
Vector3 Safe2;

void Noclip::calculate_new_position()
{

	const auto pos1 = read<uint64_t>(process_base + 0x02508CD8);	//Pointer1
	const auto pos2 = read<uint64_t>(pos1 + 0x10);	//Pointer 2
	const auto pos3 = read<uint64_t>(pos2 + 0x8);	//Pointer 3
	const auto pos4 = read<uint64_t>(pos3 + 0xD0);	//Pointer 4

	const auto yawx = read<float>(pos4 + 0x8);	//Local player yaw
	const auto yawx2 = read<float>(pos4 + 0x20); //Local player yaw2

	float foward_vector = get_foward_factor() * yawx;
	float side_vector = get_foward_factor() * yawx2;

	current_position.z += foward_vector * no_clip_speed;
	current_position.x += side_vector * no_clip_speed;
	current_position.y += (get_up_factor() / 10) * no_clip_speed;
}

bool firsttime = true;
bool calculated = false;
bool init = true;

Vector3 getpos()
{
	const auto pos1 = read<uint64_t>(process_base + 0x02508CD8);
	const auto pos2 = read<uint64_t>(pos1 + 0x10);
	const auto pos3 = read<uint64_t>(pos2 + 0x8);
	const auto pos4 = read<uint64_t>(pos3 + 0xD0);

	const auto myposx = read<float>(pos4 + 0x2C);	//Local Player X position
	const auto myposy = read<float>(pos4 + 0x30);	//Local Player Y position
	const auto myposz = read<float>(pos4 + 0x34);	//Local Player Z position

	return { myposx, myposy, myposz };
}

void Noclip::on_update()
{
	if (!no_clip)
	{
		firsttime = true;
		calculated = false;
		return;
	}

	if (firsttime)
	{
		firsttime = false;

		const auto pos1 = read<uint64_t>(process_base + 0x02508CD8);
		const auto pos2 = read<uint64_t>(pos1 + 0x10);
		const auto pos3 = read<uint64_t>(pos2 + 0x8);
		const auto pos4 = read<uint64_t>(pos3 + 0xD0);

		const auto myposx = read<float>(pos4 + 0x2C);	//Local Player X position
		const auto myposy = read<float>(pos4 + 0x30);	//Local Player Y position
		const auto myposz = read<float>(pos4 + 0x34);	//Local Player Z postion

		current_position = Vector3(myposx, myposy, myposz);	

	}

	if (GetAsyncKeyState(VK_F5) & 0x8000)
	{
		Safe1 = getpos();
	}
	else if (GetAsyncKeyState(VK_F6) & 0x8000)
	{
		Safe2 = getpos();
	}
	else if (GetAsyncKeyState(VK_F7) & 0x8000)
	{
		current_position = Safe1;
	}
	else if (GetAsyncKeyState(VK_F8) & 0x8000)
	{
		current_position = Safe2;
	}
	
	calculate_new_position();

	calculated = true;
}


inline void change_server_position(const Vector3& v)
{

	const auto pos1 = read<uint64_t>(process_base + 0x02508CD8);
	const auto pos2 = read<uint64_t>(pos1 + 0x10);
	const auto pos3 = read<uint64_t>(pos2 + 0x8);
	const auto pos4 = read<uint64_t>(pos3 + 0xD0);

	write<float>(pos4 + 0x2C, v.x);	//Overwrite your X Position with the new calculated position
	write<float>(pos4 + 0x30, v.y);	//Overwrite your Y Position
	write<float>(pos4 + 0x34, v.z);	//Overwrite your Z Position

}

void Noclip::on_fast_update()
{
	if (calculated)
	{
		change_server_position(current_position);
	}
}
