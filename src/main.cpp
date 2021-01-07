#include "giomm/application.h"
#include "main_window.h"
#include "resource_manager.h"
#include "settings_manager.h"
#include "logger.h"

#if defined(WIN32) || defined(WIN64)
#include <shellapi.h>
#include <windows.h>
#include <sphelper.h>
#include <commctrl.h>
#include <gdk/gdkwin32.h>

#define APPWM_ICONNOTIFY (WM_APP + 1)
#define APP_ICON_OPEN (WM_APP + 2)
#define APP_ICON_EXIT (WM_APP + 3)

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

namespace
{
    Glib::RefPtr<Gtk::Application> app;
    TTMainWindow* main_window;
    HWND hwnd;

#if defined(WIN32) || defined(WIN64)
    NOTIFYICONDATA nid = {};
    HMENU hMenu;
    LPCTSTR lpszClass = "__hidden__";

    void ShowContextMenu(HWND hWnd)
    {
        // Get current mouse position.
        POINT curPoint;
        GetCursorPos(&curPoint);

        // should SetForegroundWindow according
        // to original poster so the popup shows on top
        SetForegroundWindow(hWnd);

        // TrackPopupMenu blocks the app until TrackPopupMenu returns
        TrackPopupMenu(
                hMenu,
                0,
                curPoint.x,
                curPoint.y,
                0,
                hWnd,
                nullptr
        );
    }
    
    void ShowNotification(const std::string& text) {
//        HINSTANCE g_hinst = GetModuleHandle(nullptr);
//        HWND hwndToolTips = CreateWindow(TOOLTIPS_CLASS, nullptr,
//                             WS_POPUP | TTS_NOPREFIX | TTS_BALLOON,
//                             0, 0, 0, 0, nullptr, nullptr, g_hinst, nullptr);
//        if(hwndToolTips) {
//            TOOLINFO ti;
//            ti.cbSize = sizeof(ti);
//            ti.uFlags = TTF_TRANSPARENT | TTF_CENTERTIP;
//            ti.hwnd = hwnd;
//            ti.uId = 0;
//            ti.hinst = nullptr;
//            ti.lpszText = LPSTR_TEXTCALLBACK;
//            GetClientRect(hwnd, &ti.rect);
//            SendMessage(hwndToolTips, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti));
//        }
//        NOTIFYICONDATA IconData = {0};
//
//        IconData.cbSize = sizeof(IconData);
//        IconData.hWnd   = hwnd;
//        IconData.uFlags = NIF_INFO;

        HRESULT hr = StringCchCopy(nid.szInfo,
                                   ARRAYSIZE(nid.szInfo),
                                   TEXT(text.c_str()));

        if(FAILED(hr))
        {
            Logger::error("Failed to create notification");
        }
        nid.uTimeout = 15000; // in milliseconds

        Shell_NotifyIcon(NIM_MODIFY, &nid);

        Logger::info("Submitted notification");
    }

#endif

int on_command_line(const Glib::RefPtr<Gio::ApplicationCommandLine>& command_line,
                    Glib::RefPtr<Gtk::Application>& appPtr)
{
    appPtr->activate(); // Without activate() the window won't be shown.
#if defined(WIN32) || defined(WIN64)
    Logger::info("Trying to create notification icon");


    auto hIcon = static_cast<HICON>(LoadImage(nullptr,
                                              TEXT(ResourceManager::get_resource_path("icon.ico").c_str()),
                                              IMAGE_ICON,
                                              0, 0,
                                              LR_DEFAULTCOLOR | LR_SHARED | LR_DEFAULTSIZE | LR_LOADFROMFILE));

    Logger::info("Retrieved icon");

    HINSTANCE hInstance = GetModuleHandle(nullptr);
    WNDCLASS wc;

    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hbrBackground = nullptr;
    wc.hCursor = nullptr;
    wc.hIcon = nullptr;
    wc.hInstance = hInstance;
    wc.lpfnWndProc = WndProc;
    wc.lpszClassName = lpszClass;
    wc.lpszMenuName = nullptr;
    wc.style = 0;
    RegisterClass(&wc);

    hwnd = CreateWindow(lpszClass, lpszClass, WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                        nullptr, nullptr, hInstance, nullptr);

    Logger::info("Created window");

    hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, APP_ICON_OPEN, "Open");
    AppendMenu(hMenu, MF_STRING, APP_ICON_EXIT, "Exit");

    Logger::info("Created menu");

    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

    Logger::info("Set icon sizes");

    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_INFO;
    nid.uCallbackMessage = APPWM_ICONNOTIFY;
    nid.hIcon = hIcon;

    Logger::info("Initialized NOTIFYICONDATA");

    StringCchCopy(nid.szTip, ARRAYSIZE(nid.szTip), "TVTorrent");

    Logger::info("Copied TVTorrent into icon tooltip");

    bool succ = Shell_NotifyIcon(NIM_ADD, &nid);

    if(succ)
        Logger::info("Created notification icon");
    else Logger::error("Failed to create notification icon");

#endif
  return EXIT_SUCCESS;
}

} // anonymous namespace

int main(int argc, char *argv[])
{
	app = Gtk::Application::create("com.sheepkill15.tvtorrent", Gio::APPLICATION_HANDLES_COMMAND_LINE | Gio::APPLICATION_HANDLES_OPEN);
	Logger::init();
	ResourceManager::init();
	SettingsManager::init();

	Logger::info("Application initialized");

	main_window = new TTMainWindow(ShowNotification);

	Logger::info("Main window initialized");

	if(argc == 2) {
	    Logger::info(std::string("Received external uri: ") + argv[1]);
		main_window->external_torrent(argv[1]);
	}

	app->signal_command_line().connect(sigc::bind(sigc::ptr_fun(&on_command_line), app), false);
    app->hold();
	int result = app->run(*main_window);
    delete main_window;
#if defined(WIN32) || defined(WIN64)
	Shell_NotifyIcon(NIM_DELETE, &nid);
#endif
	return result;
}

void OpenWindow() {
    main_window->show();
    //app->release();
}

void HideWindow() {
    app->release();
    main_window->hide();
}

#if defined(WIN32) || defined(WIN64)
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case APPWM_ICONNOTIFY:
        {
            switch (lParam)
            {
                case WM_LBUTTONUP:
                    //...
                    if(hMenu != nullptr) {
                        UINT uild = GetMenuItemID(hMenu, 0);
                        if(uild == -1) {
                            OpenWindow();
                            goto skip;
                        }

                        SendMessage(hWnd, WM_COMMAND, uild, 0);
                    }
skip:
                    break;
                case WM_RBUTTONUP:
                    //...
                    ShowContextMenu(hWnd);
                    break;
                default:
                    break;
            }

            return 0;
        }
        case WM_COMMAND: {
            int wmld = LOWORD(wParam);
            // int mwEvent = HIWORD(wParam);
            switch (wmld) {
                case APP_ICON_OPEN:
                    OpenWindow();
                    break;
                case APP_ICON_EXIT:
                    HideWindow();
                    break;
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
#endif
