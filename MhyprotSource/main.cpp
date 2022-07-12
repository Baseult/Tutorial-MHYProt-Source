// Dear ImGui: standalone example application for DirectX 11
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

//https://github.com/ocornut/imgui/issues/707

#include "settings.h"
#include "resource.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"
#include <D3D11.h>
#include <tchar.h>
#include "ImGui/imgui_internal.h"
#include "utils/Image.h"
#include "Coding.h"
#include <thread>
#include "skCrypter.h"

// Data
static ID3D11Device* g_pd3d_device = nullptr;
static ID3D11DeviceContext* g_pd3d_device_context = nullptr;
static IDXGISwapChain* g_p_swap_chain = nullptr;
static ID3D11RenderTargetView* g_main_render_target_view = nullptr;

// Forward declarations of helper functions
bool create_device_d_3d(HWND h_wnd);
void cleanup_device_d_3d();
void create_render_target();
void cleanup_render_target();
LRESULT WINAPI wnd_proc(HWND h_wnd, UINT msg, WPARAM w_param, LPARAM l_param);

constexpr auto color_primary_hex = 0x6ecfff; //0x4cc2ff;0x3d50fa;//
const auto color_primary = hex(color_primary_hex); //0x3d50fa_h;
const auto color_primary_black = 0x111111_h;
const auto text_color = 0xe6e6e6_h;
const auto white = 0xFFFFFF_h;
const auto color_black = 0x22252e_h;
const auto white_alpha_small = hex(0xFFFFFF, 0.05f);
const auto white_alpha_medium = hex(0xFFFFFF, 0.10f);
const auto white_alpha_large = hex(0xFFFFFF, 0.25f);
const auto white_enabled = 0xA0A0A0_h;
const auto transparent = hex(0, 0.0f);;

void style_colors_blue_fire(ImGuiStyle* dst)
{
	ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
	ImVec4* colors = style->Colors;
	constexpr float radius = 5.f;
	style->TabRounding = radius;
	style->WindowRounding = radius;
	style->ChildRounding = radius;
	style->FrameRounding = radius; // checkmark and boxes
	style->PopupRounding = radius;
	style->TabRounding = radius;
	style->GrabRounding = radius;

	colors[ImGuiCol_Text] = text_color;
	colors[ImGuiCol_CheckMark] = white;
	colors[ImGuiCol_Button] = white_alpha_small;
	colors[ImGuiCol_ButtonHovered] = white_alpha_medium;
	colors[ImGuiCol_ButtonActive] = white_alpha_large;
	colors[ImGuiCol_SliderGrab] = 0x404040_h;
	colors[ImGuiCol_SliderGrabActive] = 0x606060_h;
	colors[ImGuiCol_FrameBgActive] = color_primary;
	colors[ImGuiCol_FrameBgWhite] = white_enabled;
	colors[ImGuiCol_FrameBg] = white_alpha_small;
	colors[ImGuiCol_FrameBgHovered] = white_alpha_medium;
	colors[ImGuiCol_WindowBg] = transparent;
	colors[ImGuiCol_HeaderHovered] = white_alpha_medium;
	colors[ImGuiCol_Header] = white_alpha_small;
	colors[ImGuiCol_HeaderActive] = white_alpha_large;
}

// Simple helper function to load an image into a DX11 texture with common settings
bool load_texture_from_file(const char* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
	// Load from disk into a raw RGBA buffer
	int image_width = 0;
	int image_height = 0;
	unsigned char* image_data = stbi_load(filename, &image_width, &image_height, nullptr, 4);
	if (image_data == nullptr)
		return false;

	// Create texture
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = image_width;
	desc.Height = image_height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;

	ID3D11Texture2D* pTexture = nullptr;
	D3D11_SUBRESOURCE_DATA subResource;
	subResource.pSysMem = image_data;
	subResource.SysMemPitch = desc.Width * 4;
	subResource.SysMemSlicePitch = 0;
	g_pd3d_device->CreateTexture2D(&desc, &subResource, &pTexture);

	// Create texture view
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = desc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;
	g_pd3d_device->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
	pTexture->Release();

	*out_width = image_width;
	*out_height = image_height;
	stbi_image_free(image_data);

	return true;
}

struct im_gui_texture
{
	ID3D11ShaderResourceView* texture;
	ImVec2 dimentions;
	ImVec4 image_tint;

	im_gui_texture(ID3D11ShaderResourceView* tex, const ImVec2 dim) : texture(tex), dimentions(dim)
	{
	}

	im_gui_texture(const char* file_path, const ImVec4 tint) : texture(nullptr), image_tint(tint)
	{
		int my_image_width = 0;
		int my_image_height = 0;

		bool ret = load_texture_from_file(file_path, &texture, &my_image_width, &my_image_height);
		dimentions = ImVec2(my_image_width, my_image_height);
	}

	explicit im_gui_texture(const char* file_path, const int tint = 0xFFFFFFF, const float alpha = 1.0f) : texture(
		nullptr)
	{
		int my_image_width = 0;
		int my_image_height = 0;

		bool ret = load_texture_from_file(file_path, &texture, &my_image_width, &my_image_height);
		dimentions = ImVec2(my_image_width, my_image_height);
		image_tint = hex(tint, alpha);
	}
};

void draw_texture(const im_gui_texture tex)
{
	ImGui::Image(tex.texture, tex.dimentions, ImVec2(0, 0), ImVec2(1, 1), tex.image_tint);
}


void draw_slider(const char* name, const im_gui_texture tex, float* f, const float min = 0.0, const float max = 1.0f)
{
	draw_texture(tex);
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 0));
	ImGui::SameLine();
	ImGui::SliderFloat(name, f, min, max, "");
	ImGui::PopStyleColor();
}

void draw_slider_int(const char* name, const im_gui_texture tex, int* f, const int min, const int max)
{
	draw_texture(tex);
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 0));
	ImGui::SameLine();
	ImGui::SliderInt(name, f, min, max, "");
	ImGui::PopStyleColor();
}

bool draw_button(const bool is_selected, const im_gui_texture tex)
{
	if (is_selected)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, color_primary);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color_primary);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, color_primary);
	}

	const auto ret = ImGui::ImageButtonPadding(tex.texture, tex.dimentions,
	                                           ImVec2(ImGui::GetWindowWidth() / 4 - tex.dimentions.x / 2 - 6, 0),
	                                           ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0),
	                                           is_selected ? color_primary_black : white);

	if (is_selected)
		ImGui::PopStyleColor(3);
	return ret;
}

void draw_check_color(const char* text, bool* is_checked, const char* color_text, ImVec4* color)
{
	ImGui::Checkbox(text, is_checked);

	if (*is_checked)
	{
		ImGui::SameLine();
		ImGui::ColorEdit4(color_text, reinterpret_cast<float*>(color), ImGuiColorEditFlags_NoInputs);
	}
}

void hack()
{
	if (!init_hack())
	{
		exit(1);
	}
}

// Main code
int main(int, char**)
{
	load_config();

	std::thread t1(hack);

	constexpr int window_height = 700;
	constexpr int window_width = 500;

	// Create application window
	//ImGui_ImplWin32_EnableDpiAwareness();
	const WNDCLASSEX wc = {
		sizeof(WNDCLASSEX), CS_CLASSDC, wnd_proc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
		_T("Settings"),
		nullptr
	};
	::RegisterClassEx(&wc);
	const HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("Settings"), WS_OVERLAPPEDWINDOW, 100, 100, window_width,
	                           window_height, NULL, NULL, wc.hInstance, NULL);

	HICON h_icon = LoadIcon(wc.hInstance, MAKEINTRESOURCE(MAINICON));

	SendMessage(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(h_icon));
	SendMessage(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(h_icon));

	// Initialize Direct3D
	if (!create_device_d_3d(hwnd))
	{
		printf("Failed to CreateDeviceD3D\n");
		cleanup_device_d_3d();
		::UnregisterClass(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	// Show the window
	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	const ImGuiIO& io = ImGui::GetIO();
	(void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	style_colors_blue_fire(nullptr);
	//ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(g_pd3d_device, g_pd3d_device_context);

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
	//io.Fonts->AddFontFromFileTTF("res/fonts/opensans.ttf", 20.0f);
	io.Fonts->AddFontFromFileTTF(skCrypt("res/fonts/segoeuivf.ttf"), 25.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\Veranda.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);

	//

	// Our state
	bool show_demo_window = false;
	bool show_another_window = false;
	const auto background_color = 0x111111_h;

	const auto fov_tex = im_gui_texture(skCrypt("res/img/fov.png"));

	const auto vis_tex = im_gui_texture(skCrypt("res/img/vis.png"));

	// Main loop
	bool done = false;
	while (!done)
	{
		// Poll and handle messages (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				done = true;
		}
		if (done)
			break;

		// Start the Dear ImGui frame
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		//Image image;
		//image.load("res/img/speed.png");

		//auto ptr = image.pixels;
		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		if (show_demo_window)ImGui::ShowDemoWindow(&show_demo_window);
		{
			static int tab = 0;
			static auto flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
			ImGui::SetNextWindowPos(ImVec2(0, 0));
			ImGui::SetNextWindowSize(ImVec2(window_width - 15, window_height), 0);
			ImGui::Begin(skCrypt("Settings"), nullptr, flags);

			if (draw_button(tab == 0, fov_tex))
			{
				tab = 0;
			}
			ImGui::SameLine();
			if (draw_button(tab == 1, vis_tex))
			{
				tab = 1;
			}

			if (tab == 0)
			{
				ImGui::Checkbox(skCrypt("NoClip"), &no_clip);
				if (no_clip)
				{
					ImGui::Checkbox(skCrypt("Spoof"), &spoof);
					ImGui::SliderFloat(skCrypt("NoClipSpeed"), &no_clip_speed, 0.01f, 2.0f);
				}

				ImGui::Checkbox(skCrypt("ESP"), &esp);
				if (esp)
				{
					ImGui::Checkbox(skCrypt("Tracers"), &tracers);
					ImGui::Checkbox(skCrypt("WarningTracers"), &warningtracers);
				}

				ImGui::Checkbox(skCrypt("Aimbot"), &aimbot);
				if (aimbot)
				{
					ImGui::Checkbox(skCrypt("ShowFov"), &aimbot_fov_enabled);
					ImGui::SliderInt(skCrypt("Fov"), &aimbot_fov, 10, 4000);
					ImGui::SliderFloat(skCrypt("Smoothnes"), &aimbot_speed, 0.01f, 1.0f);
					ImGui::SliderFloat(skCrypt("BulletSpeed"), &bulletspeed, 50.f, 900.f);
					ImGui::SliderFloat(skCrypt("BulletGravity"), &bulletgravity, 0.01f, 70.f);
					ImGui::SliderFloat(skCrypt("Upmove"), &upmove, -100.f, 100.f);
					//DrawSlider(skCrypt("Aimheight"), "Aimheight", &aimHeight);

					//ImGui::Checkbox(skCrypt("AutoShoot"), &autoshoot);
					ImGui::Checkbox(skCrypt("Hold"), &is_hold);
					/*if (isHold) {
						ImGui::Checkbox(skCrypt("Invert Hold"), &invertHold);
					}*/

					if (hold_key_index >= 0)
					{
						ImGui::Combo(is_hold ? skCrypt("Hold key") : skCrypt("Toggle Key"), &hold_key_index, hold_keys,
						             hold_size);
						hold_key = hold_keys_codes[hold_key_index];
					}
					else
					{
						ImGui::Text(skCrypt("Custom key used: 0x%llX"), hold_key);
					}
				}
			}
			else if (tab == 1)
			{
			}
			ImGui::End();
		}

		// 3. Show another simple window.
		if (show_another_window)
		{
			ImGui::Begin(skCrypt("Another Window"), &show_another_window);
			// Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
			ImGui::Text(skCrypt("Hello from another window!"));
			if (ImGui::Button(skCrypt("Close Me")))
				show_another_window = false;
			ImGui::End();
		}

		// Rendering
		ImGui::Render();
		const float clear_color_with_alpha[4] = {
			background_color.x * background_color.w, background_color.y * background_color.w,
			background_color.z * background_color.w, background_color.w
		};
		g_pd3d_device_context->OMSetRenderTargets(1, &g_main_render_target_view, nullptr);
		g_pd3d_device_context->ClearRenderTargetView(g_main_render_target_view, clear_color_with_alpha);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		//g_pSwapChain->Present(1, 0); // Present with vsync
		g_p_swap_chain->Present(0, 0); // Present without vsync
	}

	save_config();

	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	cleanup_device_d_3d();
	DestroyWindow(hwnd);
	::UnregisterClass(wc.lpszClassName, wc.hInstance);


	shutdown();
	exit(0);

	return 0;
}

// Helper functions

bool create_device_d_3d(const HWND h_wnd)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = h_wnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL feature_level;
	constexpr D3D_FEATURE_LEVEL feature_level_array[2] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0,};
	if (constexpr UINT create_device_flags = 0; D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, create_device_flags, feature_level_array,
	                                                                        2,
	                                                                        D3D11_SDK_VERSION, &sd, &g_p_swap_chain, &g_pd3d_device, &feature_level,
	                                                                        &g_pd3d_device_context) != S_OK)
		return false;

	create_render_target();
	return true;
}

void cleanup_device_d_3d()
{
	cleanup_render_target();
	if (g_p_swap_chain)
	{
		g_p_swap_chain->Release();
		g_p_swap_chain = nullptr;
	}
	if (g_pd3d_device_context)
	{
		g_pd3d_device_context->Release();
		g_pd3d_device_context = nullptr;
	}
	if (g_pd3d_device)
	{
		g_pd3d_device->Release();
		g_pd3d_device = nullptr;
	}
}

void create_render_target()
{
	ID3D11Texture2D* p_back_buffer;
	g_p_swap_chain->GetBuffer(0, IID_PPV_ARGS(&p_back_buffer));
	g_pd3d_device->CreateRenderTargetView(p_back_buffer, nullptr, &g_main_render_target_view);
	p_back_buffer->Release();
}

void cleanup_render_target()
{
	if (g_main_render_target_view)
	{
		g_main_render_target_view->Release();
		g_main_render_target_view = nullptr;
	}
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT im_gui_impl_win32_wnd_proc_handler(HWND h_wnd, UINT msg, WPARAM w_param, LPARAM l_param);

// Win32 message handler
LRESULT WINAPI wnd_proc(const HWND h_wnd, const UINT msg, WPARAM w_param, const LPARAM l_param)
{
	if (im_gui_impl_win32_wnd_proc_handler(h_wnd, msg, w_param, l_param))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (g_pd3d_device != nullptr && w_param != SIZE_MINIMIZED)
		{
			cleanup_render_target();
			g_p_swap_chain->ResizeBuffers(0, LOWORD(l_param), HIWORD(l_param), DXGI_FORMAT_UNKNOWN, 0);
			create_render_target();
		}
		return 0;
	case WM_SYSCOMMAND:
		if ((w_param & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProc(h_wnd, msg, w_param, l_param);
}
