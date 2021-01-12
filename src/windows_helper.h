//
// Created by simon on 2021. jan. 11..
//

#ifndef TVTORRENT_WINDOWS_HELPER_H
#define TVTORRENT_WINDOWS_HELPER_H

#include <shellapi.h>
#include <windows.h>
#include <sphelper.h>

#define APPWM_ICONNOTIFY (WM_APP + 1)
#define APP_ICON_OPEN (WM_APP + 2)
#define APP_ICON_EXIT (WM_APP + 3)

void (*show_window)();
void (*hide_window)();

NOTIFYICONDATA nid = {};
HMENU hMenu;
LPCTSTR lpszClass = "__hidden__";
HWND hwnd;

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void ShowContextMenu(HWND hWnd) {
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

void ShowNotification(const std::string &text) {

    HRESULT hr = StringCchCopy(nid.szInfo,
                               ARRAYSIZE(nid.szInfo),
                               TEXT(text.c_str()));

    if (FAILED(hr)) {
        Logger::error("Failed to create notification");
    }
    nid.uTimeout = 15000; // in milliseconds

    Shell_NotifyIcon(NIM_MODIFY, &nid);

    Logger::info("Submitted notification");
}

void delete_icon() {
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

static bool terminated = false;

WINBOOL terminate_handler(DWORD signal) {
    if(terminated) return TRUE;
    //message_queue::remove("mq");
    Logger::info(std::string("Caught termination signal: ") + std::to_string(signal));
    // DataContainer::cleanup();

    ResourceManager::create_torrent_save(DataContainer::get_groups());

    std::vector<Glib::ustring> feeds;
    for(auto feed : DataContainer::get_feeds()) {
        feeds.emplace_back(feed->GetUrl());
    }

    ResourceManager::create_feed_save(feeds, DataContainer::get_filters(), DataContainer::get_downloaded());

    SettingsManager::save();

    Logger::cleanup();
    delete_icon();
    terminated = true;
    return TRUE;
}


void SetupNotificationIcon() {
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

    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM) hIcon);
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM) hIcon);

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

    if (succ)
        Logger::info("Created notification icon");
    else Logger::error("Failed to create notification icon");

    SetConsoleCtrlHandler(terminate_handler, TRUE);
}

bool check_for_running_process() {
    HANDLE h = CreateMutex(nullptr, FALSE, "tvtorrent");
    return h != nullptr && (GetLastError() == ERROR_ALREADY_EXISTS);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case APPWM_ICONNOTIFY: {
            switch (lParam) {
                case WM_LBUTTONUP:
                    //...
                    if (hMenu != nullptr) {
                        UINT uild = GetMenuItemID(hMenu, 0);
                        if (uild == -1) {
                            show_window();
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
                    show_window();
                    break;
                case APP_ICON_EXIT:
                    hide_window();
                    break;
                default:
                    break;
            }
            break;
        }
        case WM_DESTROY:
        case WM_QUIT:
        case WM_CLOSE:
        case WM_ENDSESSION:
        case WM_QUERYENDSESSION: {
            terminate_handler(1);
            break;
        }
        default:
            break;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

#endif //TVTORRENT_WINDOWS_HELPER_H
