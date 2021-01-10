#include "giomm/application.h"
#include <gtkmm/checkbutton.h>
#include "main_window.h"
#include "resource_manager.h"
#include "settings_manager.h"
#include "logger.h"
#include <boost/interprocess/ipc/message_queue.hpp>
#include "container.h"

#if defined(WIN32) || defined(WIN64)

#include <shellapi.h>
#include <windows.h>
#include <sphelper.h>

#define APPWM_ICONNOTIFY (WM_APP + 1)
#define APP_ICON_OPEN (WM_APP + 2)
#define APP_ICON_EXIT (WM_APP + 3)

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif

namespace {
    Glib::RefPtr<Gtk::Application> app;
    TTMainWindow *main_window;

#if defined(WIN32) || defined(WIN64)
    NOTIFYICONDATA nid = {};
    HMENU hMenu;
    LPCTSTR lpszClass = "__hidden__";
    HWND hwnd;

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
#else

    void ShowNotification(const std::string &text) {

            Logger::error("Failed to create notification");
    }

#endif

    int on_command_line(const Glib::RefPtr<Gio::ApplicationCommandLine> &command_line,
                        Glib::RefPtr<Gtk::Application> &appPtr) {
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

#endif
        return EXIT_SUCCESS;
    }

    bool should_work = true;
    using namespace boost::interprocess;
#if defined(WIN64) || defined(WIN32)
    void check_for_ipc_message() {

        message_queue mq(open_or_create, "mq", 20, sizeof(char) * 500);

        while (should_work) {
            try {
                size_t recvd_size;
                unsigned int priority;
                std::string buffer;
                if (mq.get_num_msg() > 0) {
                    buffer.resize(550);
                    mq.receive(buffer.data(), sizeof(char) * 500, recvd_size, priority);
                    buffer.resize(recvd_size / sizeof(char));
                    Logger::info(std::string("Received: ") + std::to_string(recvd_size) + ": " + buffer);
                    if(recvd_size <= 10) {
                        main_window->just_show();
                    }
                    else
                        main_window->notify(buffer);
                }
            } catch (interprocess_exception &er) {
                Logger::error(er.what());
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
#endif
    void OpenWindow() {
        main_window->show();
        //app->release();
    }

    void HideWindow() {
        app->release();
        main_window->hide();
    }

    bool held = false;

    void ask_for_confirmation() {
        if (!SettingsManager::get_settings().should_ask_exit) {
            if (SettingsManager::get_settings().close_to_tray) {
                if (!held) {
                    app->hold();
                    held = true;
                }
            } else {
                HideWindow();
            }
            return;
        }

        auto builder = Gtk::Builder::create_from_file(ResourceManager::get_resource_path("tvtorrent_confirm.glade"));
        Gtk::Dialog *dialog;
        builder->get_widget("ConfirmationDialog", dialog);

        Gtk::CheckButton *ask_again;
        builder->get_widget("AskAgain", ask_again);

        int result = dialog->run();

        switch (result) {
            case Gtk::RESPONSE_YES: {
                SettingsManager::get_settings().should_ask_exit = !ask_again->get_active();
                SettingsManager::get_settings().close_to_tray = true;
                if (!held) {
                    app->hold();
                    held = true;
                }
                break;
            }
            case Gtk::RESPONSE_NO:
            default:
                SettingsManager::get_settings().should_ask_exit = !ask_again->get_active();
                SettingsManager::get_settings().close_to_tray = false;
                HideWindow();
                break;
        }

        dialog->hide();
        delete dialog;

    }

} // anonymous namespace

int main(int argc, char *argv[]) {
#if defined(WIN32) || defined(WIN64)
    ResourceManager::init();
    Logger::init();
    try {
        HANDLE h = CreateMutex(nullptr, FALSE, "tvtorrent");
        bool already_running = h != nullptr && (GetLastError() == ERROR_ALREADY_EXISTS);
        Logger::info("Checking for already running processes");
        if (already_running) {
            message_queue messageQueue(open_or_create, "mq", 20, sizeof(char) * 500);
            if (argc == 2) {
                std::string teszt(argv[1]);
                Logger::info(std::string("Preparing to send ") + std::to_string(teszt.size()) + ": " + teszt);
                messageQueue.send(teszt.data(), sizeof(char) * teszt.size(), 0);
            } else {
                std::string teszt("open");
                Logger::info(std::string("Preparing to send ") + std::to_string(teszt.size()) + ": " + teszt);
                messageQueue.send(teszt.data(), sizeof(char) * teszt.size(), 0);
            }
            Logger::info("Sent");
            Logger::cleanup();
            return 0;
        }

    } catch (interprocess_exception &e) {
        Logger::error(std::to_string(e.get_error_code()) + ": " + e.what());
        Logger::cleanup();
        return -1;
    }
    std::thread mine = std::thread([] { check_for_ipc_message(); });
#endif

    app = Gtk::Application::create("com.sheepkill15.tvtorrent",
                                   Gio::APPLICATION_HANDLES_COMMAND_LINE | Gio::APPLICATION_HANDLES_OPEN);
    SettingsManager::init();
    DataContainer::init();

    Logger::info("Application initialized");

    main_window = new TTMainWindow(ShowNotification);

    Logger::info("Main window initialized");

    if (argc == 2) {
        Logger::info(std::string("Received external uri: ") + argv[1]);
        main_window->external_torrent(argv[1]);
    }
    app->signal_command_line().connect(sigc::bind(sigc::ptr_fun(&on_command_line), app), false);
    main_window->signal_hide().connect(sigc::ptr_fun(&ask_for_confirmation));

    int result = app->run(*main_window);
    delete main_window;
#if defined(WIN32) || defined(WIN64)
    Shell_NotifyIcon(NIM_DELETE, &nid);
    should_work = false;
    mine.detach();
    message_queue::remove("mq");
#endif
    Logger::cleanup();
    DataContainer::cleanup();
    return result;
}

#if defined(WIN32) || defined(WIN64)

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case APPWM_ICONNOTIFY: {
            switch (lParam) {
                case WM_LBUTTONUP:
                    //...
                    if (hMenu != nullptr) {
                        UINT uild = GetMenuItemID(hMenu, 0);
                        if (uild == -1) {
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
