#include "pch.h"
#include "platform.h"
#include "base.h"

const vec2i DEFAULT_VIEW_SIZE(1280, 720);

enum class mouse_capture {
	FREE,
	CAPTURED,
	LOST
};

bool g_request_mouse_capture = true;
mouse_capture g_mouse_capture_state = mouse_capture::LOST;

extern const wchar_t* g_win_name;
volatile bool g_win_quit = false;
HWND g_win_hwnd;
bool g_win_focus;
int g_win_modal;
bool g_win_fullscreen;
WINDOWPLACEMENT g_win_old_placement;

HANDLE g_game_thread;

#define WM_APP_UPDATE_MOUSE_CURSOR (WM_APP + 1)

void frame_init();
void frame_init_view(vec2i view_size);
void frame_render(vec2i size);
void game_init();

DWORD WINAPI game_thread_proc(void*) {
	vec2i view_size = DEFAULT_VIEW_SIZE;

	while(!g_win_quit) {
		// recreate buffers if window size changes

		RECT rc;
		GetClientRect(g_win_hwnd, &rc);

		vec2i new_view_size(max<int>(rc.right - rc.left, 64), max<int>(rc.bottom - rc.top, 64));

		if (new_view_size != view_size) {
			view_size				= new_view_size;
			g_mouse_capture_state	= mouse_capture::LOST;

			//debug("resizing swap chain: %ix%i", view_size.x, view_size.y);

			gpu_reset(view_size);
			frame_init_view(view_size);
		}

		// sync mouse state

		if (g_win_focus && (g_request_mouse_capture || g_win_fullscreen) && (g_win_modal == 0)) {
			if (g_mouse_capture_state != mouse_capture::CAPTURED) {
				g_mouse_capture_state = mouse_capture::CAPTURED;
				SendMessage(g_win_hwnd, WM_APP_UPDATE_MOUSE_CURSOR, 0, 0);
			}
		}
		else {
			if (g_mouse_capture_state != mouse_capture::FREE) {
				g_mouse_capture_state = mouse_capture::FREE;
				SendMessage(g_win_hwnd, WM_APP_UPDATE_MOUSE_CURSOR, 0, 0);
			}
		}

		// update

		input_update();
		frame_render(view_size);

		gpu_present();
	}

	PostMessage(g_win_hwnd, WM_QUIT, 0, 0);

	return 0;
}

void game_thread_start() {
	g_game_thread = CreateThread(0, 0, game_thread_proc, 0, 0, 0);
}

void game_thread_stop() {
	g_win_quit = true;
	WaitForSingleObject(g_game_thread, INFINITE);
	CloseHandle(g_game_thread);
}

vec2i mouse_msg_pos(HWND hwnd) {
	DWORD mp = GetMessagePos();
	POINT p = { GET_X_LPARAM(mp), GET_Y_LPARAM(mp) };
	ScreenToClient(hwnd, &p);
	return { p.x, p.y };
}

int mouse_msg_button(UINT msg) {
	switch(msg) {
		default:
		case WM_LBUTTONDOWN:	return 0;
		case WM_LBUTTONUP:		return 0;
		case WM_LBUTTONDBLCLK:	return 0;
		case WM_RBUTTONDOWN:	return 1;
		case WM_RBUTTONUP:		return 1;
		case WM_RBUTTONDBLCLK:	return 1;
		case WM_MBUTTONDOWN:	return 2;
		case WM_MBUTTONUP:		return 2;
		case WM_MBUTTONDBLCLK:	return 2;
	}
}

bool mouse_msg_down(UINT msg) {
	switch(msg) {
		default:
		case WM_LBUTTONDOWN:	return true;
		case WM_LBUTTONUP:		return false;
		case WM_LBUTTONDBLCLK:	return true;
		case WM_RBUTTONDOWN:	return true;
		case WM_RBUTTONUP:		return false;
		case WM_RBUTTONDBLCLK:	return true;
		case WM_MBUTTONDOWN:	return true;
		case WM_MBUTTONUP:		return false;
		case WM_MBUTTONDBLCLK:	return true;
	}
}

void toggle_fullscreen(HWND hwnd) {
	g_win_fullscreen = !g_win_fullscreen;

	if (g_win_fullscreen) {
		GetWindowPlacement(hwnd, &g_win_old_placement);

		SetWindowLong(hwnd, GWL_STYLE, WS_OVERLAPPED | WS_VISIBLE);
		SetWindowLong(hwnd, GWL_EXSTYLE, 0);

		MONITORINFO mi = { sizeof(mi) };
		GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi);

		SetWindowPos(hwnd, 0,
				mi.rcMonitor.left, mi.rcMonitor.top,
				mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top,
				SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOCOPYBITS | SWP_SHOWWINDOW
			);
	}
	else {
		SetWindowLong(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
		SetWindowLong(hwnd, GWL_EXSTYLE, WS_EX_OVERLAPPEDWINDOW);

		SetWindowPlacement(hwnd, &g_win_old_placement);
	}
}

LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch(msg) {
		// custom

		case WM_APP_UPDATE_MOUSE_CURSOR:
			if (g_mouse_capture_state == mouse_capture::CAPTURED) {
				RECT rc;
				GetClientRect(g_win_hwnd, &rc);
				MapWindowPoints(g_win_hwnd, 0, (POINT*)&rc, 2);

				ClipCursor(&rc);
				ShowCursor(FALSE);
				SetCursor(0);
			}
			else {
				ClipCursor(0);
				ShowCursor(TRUE);
				SetCursor(LoadCursor(0, IDC_ARROW));
			}
		return 0;

		// window interaction

		case WM_CLOSE:
			g_win_quit = true;
		return 0;

		case WM_ACTIVATE:
			if (!(g_win_focus = wparam != WA_INACTIVE)) {
				input_lost_focus();
				g_win_modal = 0;
			}
		break;

		case WM_ENTERMENULOOP: g_win_modal |= 1;  break;
		case WM_EXITMENULOOP:  g_win_modal &= ~1; break;
		case WM_ENTERSIZEMOVE: g_win_modal |= 2;  break;
		case WM_EXITSIZEMOVE:  g_win_modal &= ~2; break;

		// keys

		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN: {
			input_key_event(wparam, !(lparam & 0x80000000));

			if (!(lparam & 0xC0000000)) {
				if (((wparam == 'F') && (GetKeyState(VK_CONTROL) & 0x8000)) ||
					((wparam == VK_RETURN) && (lparam & 0x20000000)) ||
					(wparam == VK_F11)
				) {
					toggle_fullscreen(hwnd);
				}

				if ((wparam == VK_F4) && (lparam & 0x20000000)) {
					g_win_quit = true;
				}
			}
		}
		return 0;

		case WM_CHAR:
		case WM_DEADCHAR:
		case WM_SYSDEADCHAR:
		return 0;

		case WM_SYSCHAR:
			if ((wparam == VK_SPACE) && (lparam & 0x20000000))
				break;
		return 0;

		// mouse

		case WM_SETCURSOR:
			if (LOWORD(lparam) == HTCLIENT) {
				if (g_mouse_capture_state == mouse_capture::CAPTURED)
					SetCursor(0);
				else
					SetCursor(LoadCursor(0, IDC_ARROW));
				return TRUE;
			}
		break;

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MBUTTONDBLCLK: {
			if (!g_input.mouse_buttons)
				SetCapture(hwnd);

			input_mouse_move_event(mouse_msg_pos(hwnd));
			input_mouse_button_event(mouse_msg_button(msg), mouse_msg_down(msg));

			if (!g_input.mouse_buttons)
				ReleaseCapture();
		}
		return 0;

		case WM_MOUSEMOVE:
			input_mouse_move_event(mouse_msg_pos(hwnd));
		return 0;

		// paint

		case WM_PAINT:
			ValidateRect(hwnd, 0);
		return 0;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	// Window

	WNDCLASSEX wc = { sizeof(wc) };

	wc.lpszClassName	= L"MainWnd";
	wc.lpfnWndProc		= window_proc;
	wc.hCursor			= LoadCursor(0, IDC_ARROW);

	RegisterClassEx(&wc);

	DWORD	style	= WS_OVERLAPPEDWINDOW;
	DWORD	styleEx = WS_EX_OVERLAPPEDWINDOW;
	RECT	rcWin	= { 0, 0, DEFAULT_VIEW_SIZE.x, DEFAULT_VIEW_SIZE.y };

	AdjustWindowRectEx(&rcWin, style, FALSE, styleEx);

	RECT rcDesk;
	GetClientRect(GetDesktopWindow(), &rcDesk);

	OffsetRect(&rcWin, ((rcDesk.right - rcDesk.left) - (rcWin.right - rcWin.left)) / 2, ((rcDesk.bottom - rcDesk.top) - (rcWin.bottom - rcWin.top)) / 2);

	g_win_hwnd = CreateWindowEx(styleEx, wc.lpszClassName, g_win_name, style, rcWin.left, rcWin.top, rcWin.right - rcWin.left, rcWin.bottom - rcWin.top, 0, 0, 0, 0);

	// systems

	gpu_init(g_win_hwnd, DEFAULT_VIEW_SIZE);
	audio_init();
	input_init();

	// game

	frame_init();
	frame_init_view(DEFAULT_VIEW_SIZE);
	game_init();
	game_thread_start();

	// main loop

	ShowWindow(g_win_hwnd, SW_SHOWNORMAL);

	for(MSG msg; GetMessage(&msg, 0, 0, 0); ) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	ShowWindow(g_win_hwnd, SW_HIDE);

	// shutdown

	game_thread_stop();
	audio_shutdown();
	gpu_shutdown();
	DestroyWindow(g_win_hwnd);

	return 0;
}