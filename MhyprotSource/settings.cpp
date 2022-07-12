#include "settings.h"
#include <fstream>
#include <iostream>
#include <string>
#include <map>

ImVec4 hex(const int hex, const float alpha)
{
	const int red = (hex >> 16) & 255;
	const int green = (hex >> 8) & 255;
	const int blue = (hex >> 0) & 255;
	return {red / 255.f, green / 255.f, blue / 255.f, alpha};
}

// handy way to convert to ImVec4 from hex
ImVec4 operator "" _h(const unsigned long long h)
{
	return hex(static_cast<int>(h));
}

int run_speed = 5;
bool no_clip = false;
float no_clip_speed = 0.25;
bool esp = true;
bool tracers = false;
bool warningtracers = false;
bool spoof = false;
int aimbot_fov = 200;
float aim_height = 0.7f;
float aimbot_speed = 0.5;
long targetobject = 0;
bool autoshoot = false;
bool aimbot = false;
float humanizer = 0.5f;
bool is_hold = true;
bool invert_hold = false;
int hold_key_index = -1;
int hold_key = VK_SPACE;
bool is_aimbot_on = false;
bool aimbot_fov_enabled = false;
float bulletspeed = 550.f;
float bulletgravity = 8.5f;
float upmove = 0.f;

float velocity;
float ttk;
float velocityreal;

constexpr auto configFileName = "config.ini";

#define NAMEOF(name) #name

bool save_config()
{
#define WRITE(variable) outfile << NAMEOF(variable) << "=" << variable << std::endl;
#define WRITE4(variable) WRITE(variable.x) WRITE(variable.y) WRITE(variable.z) WRITE(variable.w);

	std::ofstream outfile(configFileName);

	outfile.close();
	std::cout << "Saved Config.\n" << std::endl;

	return true;
}

bool load_config()
{
	std::map<std::string, std::string> values;

	std::ifstream myfile;
	myfile.open(configFileName);
	if (myfile.is_open())
	{
		std::string line;
		long long count = 0;
		while (std::getline(myfile, line))
		{
			line.erase(std::remove_if(line.begin(), line.end(), isspace),
			           line.end());
			if (line[0] == '[' || line[0] == ';' || line.empty()) // COMMETS
				continue;
			auto delimiterPos = line.find("=");
			auto name = line.substr(0, delimiterPos);
			auto value = line.substr(delimiterPos + 1);
			values[name] = value;
		}
		myfile.close();

#define READ(variable) if(values.find(NAMEOF(variable)) != values.end()) variable = atof(values[NAMEOF(variable)].c_str());
#define READ4(variable) READ(variable.x) READ(variable.y) READ(variable.z) READ(variable.w);

		std::cout << "Loaded Config.\n" << std::endl;
	}
	else
	{
		std::cout << "Failed to read config.\n" << std::endl;
	}

	return true;
}
