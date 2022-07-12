#include <cstdint>
#include "../MhyprotSource/Mhyprot/baseadress.h"
#include "Overlay.h"
#include "Vector3.h"
#include "Aimbot.h"
#include <d3dx9.h>
#include "Esp.h"

#include "settings.h"
#include "skCrypter.h"
#include <iostream>
#include <chrono>
#include <list>
#include <vector>
#include <thread>
#include <iomanip>

#include "CGraphics.h"
#include "Offsets.h"


using namespace std;
using namespace std::chrono;

D2D1::ColorF teamcolor = D2D1::ColorF(0.0f, 1.0f, 0.0f, 1.0f);
D2D1::ColorF aimcolor = D2D1::ColorF(0.0f, 0.0f, 1.0f, 1.0f);
D2D1::ColorF meins = D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f);
D2D1::ColorF impactcolor = D2D1::ColorF(1.0f, 0.0f, 0.0f, 0.5f);
D2D1::ColorF xyz = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.6f);

#define MATH_PI         (4.f)
# define M_PI           3.14159265358979323846  /* pi */
#define MATH_SQRT_TWO   (2.f /  (  1.f + MATH_PI )                   )
#define MATH_LN(x)      (       (x               ) * MATH_PI* MATH_PI)

int isplayer = 0;

std::vector<EspInfo> _espinfo = {};

constexpr float player_height = 1.75f;

//----------------------------------------------------------------

class vec2
{
public:
	float x;
	float y;

	vec2();
	vec2(float a, float b);
};

vec2::vec2()
{
	x = 0;
	y = 0;
}

vec2::vec2(const float a, const float b)
{
	x = a;
	y = b;
}

//----------------------------------------------------------------


static Vector3 rotate_point(const Vector3 point_to_rotate, const Vector3 center_point, float angle, const bool angle_in_radians = false) //Calculation for the Radar rotation
{
	if (!angle_in_radians)
		angle = static_cast<float>(angle * (M_PI / 180.f));

	const auto cos_theta = (float)cos(angle);
	const auto sin_theta = (float)sin(angle);

	auto return_vec = Vector3(
		cos_theta * (point_to_rotate.x - center_point.x) + sin_theta * (point_to_rotate.y - center_point.y),
		-sin_theta * (point_to_rotate.x - center_point.x) + cos_theta * (point_to_rotate.y - center_point.y), 0);

	return_vec = return_vec + center_point;
	return return_vec;
}


float distance(const float x1, const float y1, const float z1, const float x2, const float y2, const float z2) //Calculate Distance to the target
{
	const float d = sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2) + pow(z2 - z1, 2) * 1.0);
	return d;
}

//----------------------------------------------------------------

float isaimingatyouyaw(Vector3 enemypos, Vector3 localpos) //Check if someone is aiming at you (yaw based)
{
	Vector3 relative = enemypos - localpos;
	float Yaw_range = -atan2(relative.x, relative.z);

	return Yaw_range * 180 / M_PI;
}

float isaimingatyoupitch(Vector3 enemypos, Vector3 localpos) //Check if someone is aiming at you (pitch based)
{
	Vector3 relative = enemypos - localpos;
	float Pitch_range = -atan(relative.y);
	return Pitch_range * 180 / M_PI;
}

//----------------------------------------------------------------

bool get_entity(const uint64_t entity)
{

	EspInfo Entity = {};

	bool draw = false;

	const auto preposptr = read<uint64_t>(entity + 0x190);	//Pointer for Entity
	const auto head_position = read<Vector3>(preposptr + 0x168);	//Entity Head position
	const auto feet_position = read<Vector3>(preposptr + 0x2C);		//Entity Feet position
	if (feet_position.x == NAN || feet_position.x == -NAN || feet_position.y == NAN || feet_position.y == -NAN || feet_position.z == NAN || feet_position.z == -NAN || feet_position.z == 0 || feet_position.y == 0 || feet_position.x == 0)
	{
		return false;
	}

	if (initialize_offsets::local_player_position().DistTo(head_position) <= 5) //Too near to draw
	{
		return false;
	}

	//Add a dead and team check here

	/*const int team = read<int>(entity + 0x00);
	const bool dead = read<int>(entity + 0x00);
	if (dead)	//Check if the entity is dead or not
	{
		bool isdead = is_dead(entity);	
	}

	if (team == 0)	//Check in which team the entity is
	{
		bool inmyteam = true;
	}
	else
	{
		bool inmyteam = false;
	}*/


	//Entity.ClearName = clearname;
	//Entity.Name = name;
	//Entity.dead = isdead;
	//Entity.Health = enthp;
	Entity.Position = feet_position;
	Entity.HeadPosition = head_position;
	Entity.entity = entity;
	_espinfo.push_back(Entity);		//Add entity to list

	return true;

}

bool firsttimecomp = true;
std::vector<EspInfo> testlist;

void Esp::esp_loop()
{
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		_espinfo.clear();

		//Radar Circle
		FOverlay::draw_line(100, 100, 300, 100, D2D1::ColorF(0.0f, 1.0f, 0.0f, 1.0f)); // up
		FOverlay::draw_line(100, 300, 300, 300, D2D1::ColorF(0.0f, 1.0f, 0.0f, 1.0f)); // down
		FOverlay::draw_line(100, 100, 100, 300, D2D1::ColorF(0.0f, 1.0f, 0.0f, 1.0f)); // left
		FOverlay::draw_line(300, 100, 300, 300, D2D1::ColorF(0.0f, 1.0f, 0.0f, 1.0f)); // right

		FOverlay::draw_circle(200, 200, 2, 2, &meins);	//Radar point of local player

		const int entcount = read<uint64_t>(world + m_entitylistcount);

		for (uintptr_t i = 0; i < entcount; i++) {

			entitylist = read<uint64_t>(world + m_entitylist);

			if (const auto table_size = read<uintptr_t>(entitylist + 0x8 * i); table_size != NULL) {
				get_entity(table_size);
			}
		}

		testlist.clear();
		testlist = _espinfo;
	}
}



void Esp::esp_draw()
{
	c_graphics_instance::initializeit();

	for (const auto& entity : testlist)
	{
		try
		{
			//------------------- Coding Radar -----------------
			Vector3 localpos = initialize_offsets::local_player_position();	//Get your local player position
			Vector3 endpos = { localpos.x - entity.Position.x , localpos.z - entity.Position.z, 0 }; //offset position from entity to local player

			endpos.x *= -1;
			endpos /= 3;

			const auto yaw = read<float>(world + 0x00); //Yaw viewdirection of local player
			const auto yaw2 = read<float>(world + 0x00);  //Yaw2 viewdirection of local player
			float result = atan2(yaw, yaw2) * 180 / M_PI;
			bool ck = true;
			float circle = 0;

			if (result <= 0)
			{
				result = 360 + result;
			}

			const Vector3 screencoords = c_graphics_instance::world_to_screen(entity.Position);	//calculate where to draw the esp of the entitiy on your screen
			const Vector3 screencoordshead = c_graphics_instance::world_to_screen(entity.HeadPosition);
			
			//If you don't find the head offsets you can comment the "screencoordshead" above and uncomment the "screencoordshead" below.
			//const Vector3 screencoordshead = c_graphics_instance::world_to_screen(entity.Position + Vector3(0, player_height, 0));

			if (screencoords.x == 0 || screencoords.y == 0 || screencoords.z == 0)
				continue;

			const float local_player_height = std::abs(screencoordshead.y - screencoords.y) + std::abs(screencoordshead.x - screencoords.x); //get entity height
			const float width = local_player_height * 0.25;	//get entity width;
			const auto to = vec2(screencoordshead.x, screencoords.y);
			const float head = width * 0.3f;

			const Vector3 singlex = rotate_point(endpos, Vector3(0, 0, 0), result, false);
			const float distancetoplayer = distance(localpos.x, localpos.y, localpos.z, entity.Position.x, entity.Position.y, entity.Position.z);

			if (distancetoplayer < 300) //if player is nearer than 300 meters away then draw him on the radar
			{
				FOverlay::draw_circle(singlex.x + 200, singlex.y + 200, 2, 2, &teamcolor);	//Draw entity on radar
			}

			const std::string s = std::to_string(distancetoplayer);
			char const* pchar = s.c_str();

			//-------------------------------- Draw Box around Entity --------------------------------
			FOverlay::draw_line(to.x - width, to.y, to.x + width, to.y, teamcolor); // bottom
			FOverlay::draw_line(to.x - width, to.y - local_player_height, to.x + width, to.y - local_player_height, teamcolor); // up
			FOverlay::draw_line(to.x - width, to.y, to.x - width, to.y - local_player_height, teamcolor); // left
			FOverlay::draw_line(to.x + width, to.y, to.x + width, to.y - local_player_height, teamcolor); // right
			//----------------------------------------------------------------------------------------

			FOverlay::draw_circle(screencoordshead.x, screencoordshead.y, head, 1, &teamcolor);	//Draw circle around head
			FOverlay::draw_line(screencoords.x, screencoords.y, screencoordshead.x, screencoordshead.y, teamcolor); //Draw line from head to feet

			FOverlay::draw_text(screencoords.x, screencoords.y + width, &xyz, pchar); //Draw Distance to target


			if (Aimbot test; Aimbot::current_target.entity == entity.entity)	//Draw Aimbot Prediction ESP
			{
				Vector3 impact_coords;

				double hittimetokill;
				double hitvelocity;
				double hitdistance;

				//Yee I know this is heavy and might lag the esp.. could have done that just in aimbot thread instead but idc, optimize it yourself if you want
				const Vector3 predictpos = Aimbot::predict_impact_pos(initialize_offsets::local_player_position(), Aimbot::current_target.Position, Aimbot::current_target.velocity, impact_coords, hittimetokill, hitvelocity, hitdistance);
				const Vector3 precoord = c_graphics_instance::world_to_screen(impact_coords);
				const Vector3 screencoord = c_graphics_instance::world_to_screen(predictpos);

				if (screencoord.x == 0)
				{
					continue;
				}

				FOverlay::draw_circle(screencoord.x + 2, screencoord.y + 35, 15, 4, &aimcolor);
				FOverlay::draw_circle(precoord.x, precoord.y, 8, 4, &impactcolor);

				FOverlay::draw_line(precoord.x, precoord.y, screencoord.x + 2, screencoord.y + 35, aimcolor);
				FOverlay::draw_line(precoord.x, precoord.y, screencoords.x, screencoords.y, aimcolor);

				FOverlay::draw_text(screencoord.x - 25, screencoord.y - 5, &aimcolor, "AIM HERE");
				FOverlay::draw_text(precoord.x - 22, precoord.y + 20, &impactcolor, "IMPACT");

				const std::string tk = std::to_string(hittimetokill);
				char const* tks = tk.c_str();

				const std::string vel = std::to_string(hitvelocity);
				char const* vels = vel.c_str();

				const std::string velreal = std::to_string(hitdistance);
				char const* velr = velreal.c_str();

				FOverlay::draw_text(screencoord.x + 35, screencoord.y + 10, &aimcolor, "T:");
				FOverlay::draw_text(screencoord.x + 35, screencoord.y + 25, &aimcolor, "V:");
				FOverlay::draw_text(screencoord.x + 35, screencoord.y + 40, &aimcolor, "D:");

				FOverlay::draw_text(screencoord.x + 60, screencoord.y + 10, &aimcolor, tks);
				FOverlay::draw_text(screencoord.x + 60, screencoord.y + 25, &aimcolor, vels);
				FOverlay::draw_text(screencoord.x + 60, screencoord.y + 40, &aimcolor, velr);

			}
			//ESP

			//Tracers
			if (warningtracers)
			{

				//Bad selfcoded calculation for "Enemy is aiming at you" warning tracers. Someone can probably code this a lot better.
				float test = (isaimingatyouyaw(entity.HeadPosition, localpos) * -1) - 180;	//Tracers if someone is aiming at you

				if (test <= 0)
				{
					test = 360 + test;
				}

				auto enemyyaw = read<float>(0x08); //Yaw of entity
				auto enemyyaw2 = read<float>(0x10);	//yaw2 of entity
				float entyaw = atan2(enemyyaw, enemyyaw2) * 180 / M_PI;
				entyaw = entyaw - 90;
				if (entyaw <= 0)
				{
					entyaw = 360 + entyaw;
				}

				float diffyaw = entyaw - test;
				float diffyawlock = 100 / distancetoplayer;

				float entpitch = (isaimingatyoupitch(entity.HeadPosition, localpos));
				auto enemypitch = read<float>(0x30C);
				enemypitch = atan(enemypitch) * 180 / M_PI;

				if (entpitch <= 0)
				{
					entpitch = 360 + entpitch;
				}

				if (enemypitch <= 0)
				{
					enemypitch = 360 + enemypitch;
				}

				float diffpitch = entpitch - enemypitch;
				float diffpitchlock = 90;

				if (diffyaw <= diffyawlock && diffyaw >= -diffyawlock && diffpitch <= diffpitchlock && diffpitch >= -diffpitchlock)
				{
					FOverlay::draw_line(2560 / 2, 1440 / 2, screencoords.x, screencoords.y, teamcolor);
				}

			}
			else if (tracers)
			{
				FOverlay::draw_line(2560 / 2, 1440 / 2, screencoords.x, screencoords.y, teamcolor);
			}

			if (screencoords.x != NAN && screencoords.y != NAN && screencoords.x != -NAN && screencoords.y != -NAN)
			{
				continue;
			}

		}
		catch (...) //dumb
		{

		}
	}
}






