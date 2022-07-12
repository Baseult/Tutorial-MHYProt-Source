#include "overlay.h"
#include <functional>
#include <corecrt_math_defines.h>

static HWND win;
int FOverlay::screen_height = 0;
int FOverlay::screen_width = 0;

/*
Window functions
*/

void FOverlay::window_set_style()
{
	int i = 0;

	i = static_cast<int>(GetWindowLong(win, -20));

	SetWindowLongPtr(win, -20, i | 0x20);
}

void FOverlay::window_set_transparency()
{
	MARGINS margin;
	UINT color_key = 0;
	UINT opacity = 0;

	margin.cyBottomHeight = -1;
	margin.cxLeftWidth = -1;
	margin.cxRightWidth = -1;
	margin.cyTopHeight = -1;

	DwmExtendFrameIntoClientArea(win, &margin);

	constexpr UINT opacity_flag = 0x02;
	UINT color_key_flag = 0x01;
	color_key = 0x000000;
	opacity = 0xFF;

	SetLayeredWindowAttributes(win, color_key, opacity, opacity_flag);
}

void FOverlay::window_set_top_most()
{
	SetWindowPos(win, HWND_TOPMOST, screen_width, screen_height, screen_width, screen_height, 0x0002 | 0x0001);
}

HWND FOverlay::retrieve_window() { return win; }

constexpr MARGINS margin = {0, 0, 0, 0};

LRESULT CALLBACK window_proc(const HWND hwnd, const UINT u_msg, const WPARAM w_param, const LPARAM l_param)
{
	switch (u_msg)
	{
	case (WM_PAINT):
		DwmExtendFrameIntoClientArea(win, &margin);
		break;
	}

	return DefWindowProc(hwnd, u_msg, w_param, l_param);
}

void FOverlay::create_window()
{
	const auto lp_class_name = "espoverlay";
	const auto lp_window_name = "Baseult Overlay";

	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = window_proc;
	wc.hInstance = GetModuleHandleA(nullptr);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(RGB(0, 0, 0));
	wc.lpszClassName = lp_class_name;
	wc.lpszMenuName = lp_window_name;

	RegisterClassEx(&wc);

	win = CreateWindowExA(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT, lp_class_name, lp_window_name, WS_POPUP, 0, 0,
	                      screen_width, screen_height, nullptr, nullptr, wc.hInstance, nullptr);
}

// Hijacking method down here.

BOOL FOverlay::window_init(const bool use_nvidia_overlay)
{
	if (use_nvidia_overlay)
	{
		//win = FindWindow("WorkerW", "");
		win = FindWindow("CEF-OSC-WIDGET", "NVIDIA GeForce Overlay");
	}
	if (!win)
		create_window();

	if (!win)
		return FALSE;

	window_set_style();
	window_set_transparency();
	window_set_top_most();

	ShowWindow(win, SW_SHOW);

	return TRUE;
}

/*
Overlay functions
*/

ID2D1Factory* d2d_factory;
ID2D1HwndRenderTarget* tar;
IDWriteFactory* write_factory;
ID2D1SolidColorBrush* brush;
ID2D1SolidColorBrush* red_brush;
ID2D1SolidColorBrush* green_brush;
IDWriteTextFormat* format;

void FOverlay::d2d_shutdown()
{
	// Release
	tar->Release();
	write_factory->Release();
	brush->Release();
	red_brush->Release();
	green_brush->Release();
	d2d_factory->Release();
}

BOOL FOverlay::init_d2d() const
{
	RECT rc;

	// Initialize D2D here
	HRESULT ret = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2d_factory);
	if (FAILED(ret))
		return FALSE;

	ret =
		DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
		                    reinterpret_cast<IUnknown**>(&write_factory));
	if (FAILED(ret))
		return FALSE;

	write_factory->CreateTextFormat(
		L"Consolas", nullptr, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL, 13.0, L"en-us", &format);

	GetClientRect(retrieve_window(), &rc);

	ret = d2d_factory->CreateHwndRenderTarget(
		D2D1::RenderTargetProperties(
			D2D1_RENDER_TARGET_TYPE_DEFAULT,
			D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN,
			                  D2D1_ALPHA_MODE_PREMULTIPLIED)),
		D2D1::HwndRenderTargetProperties(
			retrieve_window(),
			D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top)),
		&tar);
	if (FAILED(ret))
		return FALSE;

	tar->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &brush);
	tar->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &red_brush);
	tar->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Green), &green_brush);

	return TRUE;
}

void FOverlay::begin_scene() { tar->BeginDraw(); }

void FOverlay::end_scene() { tar->EndDraw(); }

void FOverlay::clear_scene() { tar->Clear(); }

void FOverlay::draw_text_white(const int x, const int y, const char* str, ...)
{
	char buf[4096];
	int len = 0;
	wchar_t b[256];

	// if (!draw) // no need for it.
	//	 return;

	va_list arg_list;
	va_start(arg_list, str);
	vsnprintf(buf, sizeof(buf), str, arg_list);
	va_end(arg_list);

	len = strlen(buf);
	mbstowcs(b, buf, len);

	tar->DrawText(b, len, format, D2D1::RectF(x, y, 1920, 1080), brush,
	              D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
}

void FOverlay::draw_line(const int x1, const int y1, const int x2, const int y2, const D2D1::ColorF color)
{
	auto point1 = D2D1_POINT_2F();
	point1.x = x1;
	point1.y = y1;

	auto point2 = D2D1_POINT_2F();
	point2.x = x2;
	point2.y = y2;

	ID2D1SolidColorBrush* temp;

	tar->CreateSolidColorBrush(color, &temp);
	tar->DrawLine(point1, point2, temp);
	temp->Release();
}

void FOverlay::draw_arc(const int x, const int y, int radius, const int start_angle, const int percent, const int thickness, void* color) {
	constexpr auto precision = (2 * M_PI) / 30;
	constexpr auto step = M_PI / 180;
	const auto inner = radius - thickness;
	const auto end_angle = (start_angle + percent) * step;
	const auto start_angles = (start_angle * M_PI) / 180;

	for (; radius > inner; --radius) {
		for (auto angle = start_angles; angle < end_angle; angle += precision) {
			const auto cx = std::round(x + radius * std::cos(angle));
			const auto cy = std::round(y + radius * std::sin(angle));

			const auto cx2 = std::round(x + radius * std::cos(angle + precision));
			const auto cy2 = std::round(y + radius * std::sin(angle + precision));

			FOverlay::draw_line(cx, cy, cx2, cy2, D2D1::ColorF(0.0f, 1.0f, 0.0f, 1.0f));
		}
	}
}

void FOverlay::draw_rect(const int x1, const int y1, const int x2, const int y2, void* color) const
{
	auto rect = D2D1_RECT_F();
	rect.bottom = y1;
	rect.top = y2;
	rect.right = x2;
	rect.left = x1;

	draw_box(rect, color);
}

void FOverlay::draw_box(const int x, const int y, const float width, const float height, void* color) const
{
	auto rect = D2D1_RECT_F();
	rect.bottom = y - height / 2;
	rect.top = y + height / 2;
	rect.right = x + width / 2;
	rect.left = x - width / 2;

	draw_box(rect, color);
}

void FOverlay::draw_box(const D2D1_RECT_F rect, void* color)
{
	ID2D1SolidColorBrush* temp;

	tar->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &temp);

	tar->FillRectangle(rect, temp);
	temp->Release();
}

void FOverlay::draw_circle(const int x, const int y, const float radius, const float width, void* color)
{
	ID2D1SolidColorBrush* temp;
	auto point = D2D1_POINT_2F();
	point.x = x;
	point.y = y;

	auto elipse = D2D1_ELLIPSE();
	elipse.point = point;
	elipse.radiusX = radius;
	elipse.radiusY = radius;

	tar->CreateSolidColorBrush(*static_cast<D2D1::ColorF*>(color), &temp);
	tar->DrawEllipse(elipse, temp, width);
	temp->Release();
}


void FOverlay::draw_text(const int x, const int y, void* color, const char* str, ...)
{
	char buf[4096];
	int len = 0;
	wchar_t b[256];

	// if (!draw) // no need for it.
	//	 return;

	va_list arg_list;
	va_start(arg_list, str);
	vsnprintf(buf, sizeof(buf), str, arg_list);
	va_end(arg_list);

	len = strlen(buf);
	mbstowcs(b, buf, len);
	ID2D1SolidColorBrush* temp;

	tar->CreateSolidColorBrush(*static_cast<D2D1::ColorF*>(color), &temp);

	tar->DrawText(b, len, format, D2D1::RectF(x, y, 2560, 1440), temp,
	              D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
	temp->Release();
}

void FOverlay::draw_text_red(const int x, const int y, const char* str, ...)
{
	char buf[4096];
	int len = 0;
	wchar_t b[256];

	// if (!draw) // no need for it.
	//	 return;

	va_list arg_list;
	va_start(arg_list, str);
	vsnprintf(buf, sizeof(buf), str, arg_list);
	va_end(arg_list);

	len = strlen(buf);
	mbstowcs(b, buf, len);

	tar->DrawText(b, len, format, D2D1::RectF(x, y, 1920, 1080), red_brush,
	              D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
}

void FOverlay::draw_text_green(const int x, const int y, const char* str, ...)
{
	char buf[4096];
	int len = 0;
	wchar_t b[256];

	// if (!draw) // no need for it.
	//	 return;

	va_list arg_list;
	va_start(arg_list, str);
	vsnprintf(buf, sizeof(buf), str, arg_list);
	va_end(arg_list);

	len = strlen(buf);
	mbstowcs(b, buf, len);

	tar->DrawText(b, len, format, D2D1::RectF(x, y, 1920, 1080), green_brush,
	              D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
}

void FOverlay::clear_screen()
{
	begin_scene();
	clear_scene();
	end_scene();
}
