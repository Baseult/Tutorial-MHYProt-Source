
// THIS SOURCE HAS BEEN UPDATED AND MOFIED BY BASEULT - ORIGINAL SOURCE BY BLUEFIRE1337's PALADINS CHEAT

#include <iostream>
#include <random>
#include <cstdint>
#include "d3d9.h"
#include "d3dx9.h"

#include "Overlay.h"
#include <thread>
#include "Mhyprot/mhyprot.hpp"
#include "skCrypter.h"
#include "settings.h"
#include "Mhyprot/baseadress.h"
#include "NoClip.h"
#include "Esp.h"
#include "Aimbot.h"
#include "Offsets.h"

uint64_t game;
uint32_t width;
uint32_t height;
FOverlay* g_overlay;

float screen_center_x;
float screen_center_y;

const LPCSTR window_class_name = "Game Title Name"; //Searched for the Window Name of the Game

bool use_nvidia_overlay = true; // change this if you have problems with overlay
bool rendering = true;

static void getentities()
{
	Esp::esp_loop();
}

static void render(const FOverlay* overlay)
{
	while (rendering)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		FOverlay::begin_scene();
		FOverlay::clear_scene();

		if (esp)
		{
			Esp::esp_draw();
		}

		if (aimbot)
		{

			Aimbot::initialize();
		}


		if (aimbot_fov_enabled)
		{
			

			//g_overlay->draw_circle(ScreenCenterX, ScreenCenterY, (float)aimbotFov, 1.0f, &Fovcolor); //If you want circle FOV instead
			//g_overlay->draw_circle(ScreenCenterX, ScreenCenterY, (float)aimbotFov, 1.0f, &Fovcolor);


			FOverlay::draw_line(screen_center_x - static_cast<float>(aimbot_fov),
			                    screen_center_y + static_cast<float>(aimbot_fov),
			                    screen_center_x + static_cast<float>(aimbot_fov),
			                    screen_center_y + static_cast<float>(aimbot_fov),
			                    D2D1::ColorF(1.0f, 0.0f, 0.0f, 0.5f)); // bottom
			FOverlay::draw_line(screen_center_x - static_cast<float>(aimbot_fov),
			                    screen_center_y - static_cast<float>(aimbot_fov),
			                    screen_center_x + static_cast<float>(aimbot_fov),
			                    screen_center_y - static_cast<float>(aimbot_fov),
			                    D2D1::ColorF(1.0f, 0.0f, 0.0f, 0.5f)); // up
			FOverlay::draw_line(screen_center_x - static_cast<float>(aimbot_fov),
			                    screen_center_y - static_cast<float>(aimbot_fov),
			                    screen_center_x - static_cast<float>(aimbot_fov),
			                    screen_center_y + static_cast<float>(aimbot_fov),
			                    D2D1::ColorF(1.0f, 0.0f, 0.0f, 0.5f)); // left
			FOverlay::draw_line(screen_center_x + static_cast<float>(aimbot_fov),
			                    screen_center_y - static_cast<float>(aimbot_fov),
			                    screen_center_x + static_cast<float>(aimbot_fov),
			                    screen_center_y + static_cast<float>(aimbot_fov),
			                    D2D1::ColorF(1.0f, 0.0f, 0.0f, 0.5f)); // right
		}

		FOverlay::end_scene();
	}
}

static void noclip()
{
	while (true)
	{
		if (no_clip)
		{
			Noclip::on_fast_update();
		}
	}
}

static void noclip2()
{
	while (true)
	{

			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			Noclip::on_update();
		
	}
}

void aimbot_key() {
	while (true) {

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	
			bool should_run = false;

			if (is_hold) {
				should_run = (GetKeyState(hold_key) & 0x8000);
				if (invert_hold) should_run = !should_run;
			}
			else {
				should_run = (GetKeyState(hold_key) == 1);
			}

			if (!should_run) {
				Sleep(10);
			}

			no_clip = should_run;
		
	}
}

bool initoffsets = false;
static void init(FOverlay* overlay)
{
	// Initialize the window
	if (!FOverlay::window_init(use_nvidia_overlay))
		return;

	// D2D Failed to initialize?
	if (!overlay->init_d2d())
		return;

	if (!initoffsets)
	{
		initoffsets = true;
		initialize_offsets::init_offsets();
	}

	// render loop
	std::thread r(render, overlay);
	std::thread n(noclip);
	std::thread n2(noclip2);
	std::thread k(aimbot_key);

	r.join(); // threading
	n.join();
	n2.join();
	k.join();

	FOverlay::d2d_shutdown(); //shutdown
}

void shutdown()
{
	FOverlay::d2d_shutdown();
}

LONG WINAPI simplest_crash_handler(EXCEPTION_POINTERS* exception_info)
{
	std::cout << skCrypt("[!!] Crash at addr 0x") << exception_info->ExceptionRecord->ExceptionAddress <<
		skCrypt(" by 0x") << std::hex << exception_info->ExceptionRecord->ExceptionCode << std::endl;
	return EXCEPTION_EXECUTE_HANDLER;
}

bool init_hack()
{
	SetConsoleTitle(skCrypt("BLUEFIRE1337's MhyProt Source - Modified by Baseult"));
	SetUnhandledExceptionFilter(simplest_crash_handler);
	//initTrace();

	system(skCrypt("sc stop mhyprot2")); // RELOAD DRIVER JUST IN CASE
	system(skCrypt("CLS")); // CLEAR

	auto process_name = skCrypt("game.exe");
	const auto process_id = get_process_id(process_name);
	if (!process_id)
	{
		printf(skCrypt("[!] process \"%s\ was not found\n"), process_name);
		return false;
	}

	printf(skCrypt("[+] %s (%d)\n"), process_name, process_id);

	//
	// initialize its service, etc
	//
	if (!mhyprot::init())
	{
		printf(skCrypt("[!] failed to initialize vulnerable driver\n"));
		return false;
	}

	if (!mhyprot::driver_impl::driver_init(
		false, // print debug
		false // print seedmap
	))
	{
		printf(skCrypt("[!] failed to initialize driver properly\n"));
		mhyprot::unload();
		return false;
	}
	process_base = get_process_base(process_id);
	if (!process_base)
	{
		printf(skCrypt("[!] failed to get baseadress\n"));
		mhyprot::unload();
		return false;
	}

	//printf("[+] Game Base is 0x%llX\n", process_base);
	const auto [e_magic, e_cblp, e_cp, e_crlc, e_cparhdr, e_minalloc, e_maxalloc, e_ss, e_sp, e_csum, e_ip, e_cs, e_lfarlc, e_ovno, e_res, e_oemid, e_oeminfo, e_res2, e_lfanew] = read<IMAGE_DOS_HEADER>(process_base);
	printf(skCrypt("[+] Game header Magic is 0x%llX\n"), e_magic);
	if (e_magic != 0x5A4D)
	{
		printf(skCrypt("[!] Game header Magic should be 0x5A4D\n"));
	}

	RECT desktop;
	// Get a handle to the desktop window
	const HWND h_desktop = GetDesktopWindow();
	// Get the size of screen to the variable desktop
	GetWindowRect(h_desktop, &desktop);
	const HDC monitor = GetDC(h_desktop);

	const int current = GetDeviceCaps(monitor, VERTRES);
	const int total = GetDeviceCaps(monitor, DESKTOPVERTRES);

	FOverlay::screen_width = (desktop.right - desktop.left) * total / current;
	FOverlay::screen_height = (desktop.bottom - desktop.top) * total / current;

	screen_center_x = FOverlay::screen_width / 2.f;
	screen_center_y = FOverlay::screen_height / 2.f;
	g_overlay = {nullptr};

	init(g_overlay);

	if (const HWND h_target_window = FindWindow(window_class_name, nullptr); !h_target_window)
	{
		printf("Error : Could not find window class name : %s", window_class_name);
	}

	return true;
}
