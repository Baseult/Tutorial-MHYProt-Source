#include "Inputs.h"

//#include "Menu.h"

#include <iostream>

//extern LRESULT ImGui_ImplDX11_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

InputSys::InputSys()
{
	m_hTargetWindow = nullptr;
	m_originalWndProc = 0;
}

InputSys::~InputSys()
{
	if (m_originalWndProc)
		SetWindowLongPtr(m_hTargetWindow, GWLP_WNDPROC, m_originalWndProc);
	m_originalWndProc = 0;
}

void InputSys::initialize(const HWND h_target_window)
{
	m_hTargetWindow = h_target_window;
	TCHAR class_name[256];
	GetClassName(m_hTargetWindow, class_name, 256);
	std::cout << "Class Name is : " << class_name << std::endl;

	printf("[Inputs] m_hTargetWindow = %p\n", m_hTargetWindow);
	if (!m_hTargetWindow)
	{
		std::cout << "[Inputs] [ERROR] m_hTargetWindow failed\n";
		return;
	}

	m_originalWndProc = SetWindowLongPtr(m_hTargetWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(wnd_proc));

	if (m_originalWndProc)
		std::cout << "[Inputs] [SUCCES]\n";
	else
		std::cout << "[Inputs] [FAILED]\n";
}

void InputSys::shutdown()
{
	if (m_originalWndProc)
		SetWindowLongPtr(m_hTargetWindow, GWLP_WNDPROC, m_originalWndProc);
	m_originalWndProc = 0;
}

LRESULT __stdcall InputSys::wnd_proc(const HWND h_wnd, const UINT msg, const WPARAM w_param, const LPARAM l_param)
{
	Get().process_message(msg, w_param, l_param);

	//if (Menu::IsVisible()) {
	//	ImGui_ImplDX9_WndProcHandler(hWnd, msg, wParam, lParam);
	//}
	//ImGui_ImplDX11_WndProcHandler(hWnd, msg, wParam, lParam);
	return CallWindowProcW(reinterpret_cast<WNDPROC>(Get().m_originalWndProc), h_wnd, msg, w_param, l_param);
}

bool InputSys::process_message(const UINT u_msg, const WPARAM w_param, const LPARAM l_param)
{
	switch (u_msg)
	{
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_LBUTTONDBLCLK:
	case WM_XBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
	case WM_XBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_LBUTTONUP:
	case WM_XBUTTONUP:
		return process_mouse_message(u_msg, w_param);
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		return process_keybd_message(u_msg, w_param);
	default:
		return false;
	}
}

bool InputSys::process_mouse_message(const UINT u_msg, const WPARAM w_param)
{
	auto key = VK_LBUTTON;
	auto state = KeyState::None;
	switch (u_msg)
	{
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
		state = u_msg == WM_MBUTTONUP ? KeyState::Up : KeyState::Down;
		key = VK_MBUTTON;
		break;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		state = u_msg == WM_RBUTTONUP ? KeyState::Up : KeyState::Down;
		key = VK_RBUTTON;
		break;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
		state = u_msg == WM_LBUTTONUP ? KeyState::Up : KeyState::Down;
		key = VK_LBUTTON;
		break;
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
		state = u_msg == WM_XBUTTONUP ? KeyState::Up : KeyState::Down;
		key = (HIWORD(w_param) == XBUTTON1 ? VK_XBUTTON1 : VK_XBUTTON2);
		break;
	default:
		return false;
	}

	if (state == KeyState::Up && m_iKeyMap[key] == KeyState::Down)
		m_iKeyMap[key] = KeyState::Pressed;
	else
		m_iKeyMap[key] = state;
	return true;
}

bool InputSys::process_keybd_message(const UINT u_msg, const WPARAM w_param)
{
	const auto key = w_param;
	auto state = KeyState::None;

	switch (u_msg)
	{
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		state = KeyState::Down;
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		state = KeyState::Up;
		break;
	default:
		return false;
	}

	if (state == KeyState::Up && m_iKeyMap[static_cast<int>(key)] == KeyState::Down)
	{
		m_iKeyMap[static_cast<int>(key)] = KeyState::Pressed;

		if (const auto& hotkey_callback = m_Hotkeys[key])
			hotkey_callback();
	}
	else
	{
		m_iKeyMap[static_cast<int>(key)] = state;
	}

	return true;
}

KeyState InputSys::get_key_state(const std::uint32_t vk) const
{
	return m_iKeyMap[vk];
}

bool InputSys::is_key_down(const std::uint32_t vk) const
{
	return m_iKeyMap[vk] == KeyState::Down;
}

bool InputSys::was_key_pressed(const std::uint32_t vk)
{
	if (m_iKeyMap[vk] == KeyState::Pressed)
	{
		m_iKeyMap[vk] = KeyState::Up;
		return true;
	}
	return false;
}

void InputSys::register_hotkey(const std::uint32_t vk, const std::function<void(void)>& f)
{
	m_Hotkeys[vk] = f;
}

void InputSys::remove_hotkey(const std::uint32_t vk)
{
	m_Hotkeys[vk] = nullptr;
}

void InputSys::move_mouse(int x, int y)
{
	INPUT input = {0};
	input.type = INPUT_MOUSE;
	input.mi.dx = static_cast<LONG>(x);
	input.mi.dy = static_cast<LONG>(y);
	input.mi.dwFlags = MOUSEEVENTF_MOVE;
	SendInput(1, &input, sizeof(INPUT));
}

void InputSys::left_down()
{
	INPUT input = {0};
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
	SendInput(1, &input, sizeof(INPUT));
}

void InputSys::left_up()
{
	INPUT input = {0};
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
	SendInput(1, &input, sizeof(INPUT));
}
