#pragma once

#include <winuser.rh>
#include "ImGui/imgui.h"

ImVec4 hex(int hex, float alpha = 1.0);
ImVec4 operator "" _h(unsigned long long h);
#define AIMBOT
#define AIMBOT_PREDICTION
#define NORECOIL
//#define TRIGGERBOT
#define GLOW
#define THIRDPERSON
#define NAME_ESP
#define ESP
//#define BONES
#define HP_NAME
#define HP_BAR

extern int run_speed;
extern bool no_clip;
extern float no_clip_speed;
extern bool esp;
extern bool tracers;
extern int aimbot_fov;
extern float aim_height;
extern float aimbot_speed;
extern float bulletspeed;
extern float bulletgravity;
extern bool spoof;
extern long targetobject;
extern bool aimbot;
extern float humanizer;
extern bool autoshoot;
extern bool warningtracers;
extern bool is_hold;
extern bool invert_hold;
extern int hold_key_index;
extern int hold_key;
extern bool is_aimbot_on;
extern bool aimbot_fov_enabled;
extern float upmove;
extern float velocityreal;


extern float velocity;
extern float ttk;

constexpr int hold_size = 15;
static const char* hold_keys[hold_size]{
	"Left mouse button",
	"Right mouse button",
	"Middle mouse button",
	"TAB key",
	"SHIFT key",
	"CTRL key",
	"ALT key",
	"DEL key",
	"INS key",
	"Numeric keypad 0 key",
	"NUM LOCK key",
	"Left SHIFT key",
	"Right SHIFT key",
	"Left CONTROL key",
	"Right CONTROL key",
};

static constexpr int hold_keys_codes[hold_size]{
	VK_LBUTTON,
	VK_RBUTTON,
	VK_MBUTTON,
	VK_TAB,
	VK_SHIFT,
	VK_CONTROL,
	VK_MENU,
	VK_DELETE,
	VK_INSERT,
	VK_NUMPAD0,
	VK_NUMLOCK,
	VK_LSHIFT,
	VK_RSHIFT,
	VK_LCONTROL,
	VK_RCONTROL,
};

bool save_config();
bool load_config();
