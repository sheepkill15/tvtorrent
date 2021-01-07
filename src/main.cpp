#include "giomm/application.h"
#include "main_window.h"
#include "resource_manager.h"
#include "settings_manager.h"
#include "logger.h"
#include <shellapi.h>
#include <windows.h>
#include <sphelper.h>

#define APPWM_ICONNOTIFY (WM_APP + 1)


#if defined(WIN32) || defined(WIN64)
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

namespace
{
int on_command_line(const Glib::RefPtr<Gio::ApplicationCommandLine>& command_line,
                    Glib::RefPtr<Gtk::Application>& app)
{
    app->activate(); // Without activate() the window won't be shown.
#if defined(WIN32) || defined(WIN64)
    Logger::info("Trying to create notification icon");

    NOTIFYICONDATA nid = {};

    LPCTSTR lpszClass = "__hidden__";
    auto hIcon = static_cast<HICON>(LoadImage(nullptr,
                                              TEXT(ResourceManager::get_resource_path("icon.ico").c_str()),
                                              IMAGE_ICON,
                                              0, 0,
                                              LR_DEFAULTCOLOR | LR_SHARED | LR_DEFAULTSIZE | LR_LOADFROMFILE));

    Logger::info("Retrieved icon");

    HINSTANCE hInstance = GetModuleHandle(nullptr);

    WNDCLASS wc;
    HWND hwnd;

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

    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

    Logger::info("Set icon sizes");

    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
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
	auto app = Gtk::Application::create("com.sheepkill15.tvtorrent", Gio::APPLICATION_HANDLES_COMMAND_LINE | Gio::APPLICATION_HANDLES_OPEN);
	Logger::init();
	ResourceManager::init();
	SettingsManager::init();

	Logger::info("Application initialized");

	TTMainWindow main_window;

	Logger::info("Main window initialized");

	if(argc == 2) {
	    Logger::info(std::string("Received external uri: ") + argv[1]);
		main_window.external_torrent(argv[1]);
	}

	app->signal_command_line().connect(sigc::bind(sigc::ptr_fun(&on_command_line), app), false);

	return app->run(main_window);
}

#if defined(WIN32) || defined(WIN64)
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case APPWM_ICONNOTIFY:
        {
            switch (lParam)
            {
                case WM_LBUTTONUP:
                    //...
                    Logger::info("Left Clicked!");
                    break;
                case WM_RBUTTONUP:
                    //...
                    Logger::info("Right clicked!");
                    break;
            }

            return 0;
        }

            //...
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
#endif
