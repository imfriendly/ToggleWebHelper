#include <Windows.h>
#include <TlHelp32.h>
#include <commctrl.h>
#include <tchar.h>

#define	WM_USER_SHELLICON WM_USER + 1
#define WM_TASKBAR_CREATE RegisterWindowMessage(_T("TaskbarCreated"))

NOTIFYICONDATA NID;

HANDLE thread_handle = NULL;

static void suspend_main_thread() {
	auto window = FindWindowW(L"vguiPopupWindow", L"Untitled");
	if (window == NULL) {
		return;
	}

	auto window_thread_id = GetWindowThreadProcessId(window, NULL);
	thread_handle = OpenThread(THREAD_SUSPEND_RESUME, FALSE, window_thread_id);
	if (thread_handle == NULL) {
		return;
	}

	SuspendThread(thread_handle);
}

static void resume_main_thread() {
	if (thread_handle == NULL) {
		return;
	}

	ResumeThread(thread_handle);
	CloseHandle(thread_handle);
	thread_handle = NULL;
}

static void terminate_steamwebhelper() {
	PROCESSENTRY32 process_entry;
	process_entry.dwSize = sizeof(PROCESSENTRY32);
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (Process32First(snapshot, &process_entry) == TRUE) {
		do {
			if (wcscmp(process_entry.szExeFile, L"steamwebhelper.exe") == 0) {
				HANDLE process_handle = OpenProcess(PROCESS_TERMINATE, FALSE, process_entry.th32ProcessID);
				if (process_handle != NULL) {
					TerminateProcess(process_handle, 0);
					CloseHandle(process_handle);
				}
			}
		} while (Process32Next(snapshot, &process_entry) == TRUE);
	}
	CloseHandle(snapshot);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if (message == WM_TASKBAR_CREATE) {
		Shell_NotifyIcon(NIM_ADD, &NID);
	}

	switch (message) {
	case WM_USER_SHELLICON:
		switch (LOWORD(lParam)) {
		case WM_RBUTTONDOWN:
		case WM_LBUTTONDOWN:
			POINT lpClickPoint;
			GetCursorPos(&lpClickPoint);
			HMENU hPopMenu = CreatePopupMenu();
			AppendMenu(hPopMenu, MF_STRING, 1, L"Disable steamwebhelper");
			AppendMenu(hPopMenu, MF_STRING, 2, L"Enable steamwebhelper");
			AppendMenu(hPopMenu, MF_STRING, 3, L"Exit");
			SetForegroundWindow(hWnd);
			TrackPopupMenu(hPopMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON, lpClickPoint.x, lpClickPoint.y, 0, hWnd, NULL);
			PostMessage(hWnd, WM_NULL, 0, 0);
			DestroyMenu(hPopMenu);
			break;
		}
		break;
	case WM_COMMAND:
		switch (wParam) {
		case 1:
			suspend_main_thread();
			terminate_steamwebhelper();
			break;
		case 2:
			resume_main_thread();
			break;
		case 3:
			Shell_NotifyIcon(NIM_DELETE, &NID);
			PostQuitMessage(0);
			break;
		}
		break;
	case WM_DESTROY:
		Shell_NotifyIcon(NIM_DELETE, &NID);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

BOOL WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

	// Make window using CreateWindowEx and has WndProc callback.
	WNDCLASS wc;
	HWND hWnd;
	MSG msg;

	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = L"ToggleWebHelper";
	RegisterClass(&wc);

	hWnd = CreateWindowEx(0, L"ToggleWebHelper", L"ToggleWebHelper", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 200, 100, NULL, NULL, hInstance, NULL);
	ShowWindow(hWnd, SW_HIDE);

	// tray icon settings
	NID.cbSize = sizeof(NOTIFYICONDATA);
	NID.hWnd = (HWND)hWnd;
	NID.uID = 0;
	NID.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	lstrcpy(NID.szTip, L"Toggle Steam Web Helper");
	NID.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	NID.uCallbackMessage = WM_USER_SHELLICON;

	// Display tray icon
	if (!Shell_NotifyIcon(NIM_ADD, &NID)) {
		MessageBox(NULL, L"Systray Icon Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	// Message Loop
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}