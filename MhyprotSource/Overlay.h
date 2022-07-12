#define _CRT_SECURE_NO_WARNINGS

#ifndef FOverlay_H
#define FOverlay_H

#include <windows.h>
#include <stdio.h>
#include <dwmapi.h>
#include <d2d1.h>
#include <dwrite.h>
#pragma comment(lib, "Dwrite")

#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "d2d1.lib")


#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS

class FOverlay
{
public:
	static int screen_height;
	static int screen_width;
	static void window_set_style();
	static void window_set_transparency();
	static void window_set_top_most();
	static HWND retrieve_window();
	static void create_window();
	static BOOL window_init(bool use_nvidia_overlay);
	static void d2d_shutdown();
	BOOL init_d2d() const;
	static void begin_scene();
	static void end_scene();
	static void clear_scene();
	static void draw_text_white(int x, int y, const char* str, ...);
	static void draw_text_red(int x, int y, const char* str, ...);
	static void draw_text_green(int x, int y, const char* str, ...);
	static void clear_screen();
	static void draw_text(int x, int y, void* color, const char* str, ...);
	static void draw_line(int x1, int y1, int x2, int y2, D2D1::ColorF color);
	static void draw_arc(const int x, const int y, int radius, const int start_angle, const int percent, const int thickness, void* color);
	void draw_rect(int x1, int y1, int x2, int y2, void* color) const;
	void draw_box(int x, int y, float width, float height, void* color) const;
	static void draw_box(D2D1_RECT_F rect, void* color);
	static void draw_circle(int x, int y, float radius, float width, void* color);
};

#endif
