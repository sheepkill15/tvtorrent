#include "giomm/application.h"
#include <gtkmm/checkbutton.h>
#include "main_window.h"
#include "resource_manager.h"
#include "settings_manager.h"
#include "logger.h"
#include <boost/interprocess/ipc/message_queue.hpp>
#include "container.h"
#include "updater.h"

#if defined(WIN32) || defined(WIN64)

#include "windows_helper.h"

#endif

namespace {
    Glib::RefPtr<Gtk::Application> app;
    TTMainWindow *main_window;
    Glib::Dispatcher delayed_window_creater;
    Glib::Dispatcher delayed_notifier;

    std::string last_message;
    int on_command_line(const Glib::RefPtr<Gio::ApplicationCommandLine> &command_line,
                        Glib::RefPtr<Gtk::Application> &appPtr) {
        appPtr->activate(); // Without activate() the window won't be shown.
        SetupNotificationIcon();
        return EXIT_SUCCESS;
    }

    void HideWindow() {
        if(main_window != nullptr) {
            main_window->hide();
            delete main_window;
            main_window = nullptr;
        }
    }

    void CloseWindow() {
        HideWindow();
        app->release();
    }


    bool held = false;
    void ask_for_confirmation() {
        if (!SettingsManager::get_settings().should_ask_exit) {
            if (SettingsManager::get_settings().close_to_tray) {
                if (!held) {
                    app->hold();
                    held = true;
                }
                HideWindow();
            } else {
                CloseWindow();
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
                CloseWindow();
                break;
        }

        dialog->hide();
        delete dialog;

    }

    void OpenWindow() {
        if(main_window == nullptr ){
            main_window = new TTMainWindow();
            main_window->signal_hide().connect(sigc::ptr_fun(&ask_for_confirmation));
        }
        main_window->show();
        //app->release();
    }


    void NotifyDialog() {
        OpenWindow();
        main_window->notify(last_message);
    }

    bool should_work = true;
    using namespace boost::interprocess;
    void check_for_ipc_message() {

        message_queue mq(open_or_create, "mq", 100, sizeof(char) * 1000);

        while (should_work) {
            try {
                size_t recvd_size;
                unsigned int priority;
                std::string buffer;
                if (mq.get_num_msg() > 0) {
                    buffer.resize(1010);
                    mq.receive(buffer.data(), sizeof(char) * 1000, recvd_size, priority);
                    buffer.resize(recvd_size / sizeof(char));
                    Logger::info(std::string("Received: ") + std::to_string(recvd_size) + ": " + buffer);
                    if(recvd_size <= 10) {
                        delayed_window_creater.emit();
                    }
                    else {
                        last_message = buffer;
                        delayed_notifier.emit();
                    }
                }
            } catch (interprocess_exception &er) {
                Logger::error(er.what());
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

} // anonymous namespace

int main(int argc, char *argv[]) {
    ResourceManager::init();
    Logger::init();
    show_window = OpenWindow;
    hide_window = CloseWindow;
    try {
        bool already_running = check_for_running_process();
        Logger::info("Checking for already running processes");
        if (already_running) {
            message_queue messageQueue(open_or_create, "mq", 100, sizeof(char) * 1000);
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

    app = Gtk::Application::create("com.sheepkill15.tvtorrent",
                                   Gio::APPLICATION_HANDLES_COMMAND_LINE | Gio::APPLICATION_HANDLES_OPEN);
    SettingsManager::init();
    DataContainer::init();
    DataContainer::get_manager().set_notify_callback(ShowNotification);
    delayed_window_creater.connect(sigc::ptr_fun(&OpenWindow));
    delayed_notifier.connect(sigc::ptr_fun(&NotifyDialog));

    Updater::Start();

    Logger::info("Application initialized");

    main_window = new TTMainWindow();

    Logger::info("Main window initialized");

    if (argc == 2) {
        Logger::info(std::string("Received external uri: ") + argv[1]);
        last_message = argv[1];
        delayed_notifier.emit();
    }
    app->signal_command_line().connect(sigc::bind(sigc::ptr_fun(&on_command_line), app), false);
    main_window->signal_hide().connect(sigc::ptr_fun(&ask_for_confirmation));

    int result = app->run(*main_window);
    delete main_window;
    should_work = false;
    mine.detach();
    message_queue::remove("mq");
    Updater::ForceFinish();
    DataContainer::cleanup();
    Logger::cleanup();
    delete_icon();
    return result;
}