#pragma once
#include <Windows.h>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>

#include "Singleton.h"

enum class KeyState
{
	None = 1,
	Down,
	Up,
	Pressed /*Down and then up*/
};

DEFINE_ENUM_FLAG_OPERATORS(KeyState);

class InputSys
	: public Singleton<InputSys>
{
	friend class Singleton<InputSys>;

	InputSys();
	~InputSys();

public:
	void initialize(HWND h_target_window);
	void shutdown();

	HWND GetMainWindow() const { return m_hTargetWindow; }

	KeyState      get_key_state(uint32_t vk) const;
	bool          is_key_down(uint32_t vk) const;
	bool          was_key_pressed(uint32_t vk);

	void register_hotkey(uint32_t vk, const std::function<void(void)>& f);
	void remove_hotkey(uint32_t vk);

	static void move_mouse(int x, int y);

	static void left_down();
	static void left_up();

private:
	static LRESULT WINAPI wnd_proc(HWND h_wnd, UINT msg, WPARAM w_param, LPARAM l_param);

	bool process_message(UINT u_msg, WPARAM w_param, LPARAM l_param);
	bool process_mouse_message(const UINT u_msg, const WPARAM w_param);
	bool process_keybd_message(const UINT u_msg, const WPARAM w_param);


	HWND            m_hTargetWindow;
	LONG_PTR        m_originalWndProc;
	KeyState		m_iKeyMap[256]{};

	std::function<void(void)> m_Hotkeys[256];
};