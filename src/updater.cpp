//
// Created by simon on 2021. jan. 24..
//

#include "updater.h"
#include "feed.h"
#include "logger.h"
#include "resource_manager.h"
#include "downloader.h"
#include <gtkmm/dialog.h>

#if defined(WIN32) || defined(WIN64)

#include <shellapi.h>
#endif

void Updater::CheckUpdates() {
    Logger::watcher w("Checking for updates");
    std::string buffer;
    Downloader::fetch_no_alloc("https://api.github.com/repos/sheepkill15/tvtorrent/tags", buffer);
    Json::Value root;
    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    std::string errs;
    std::stringstream buffer_stream(buffer);

    bool ok = Json::parseFromStream(builder, buffer_stream, &root, &errs);
    if(ok) {
        std::string tagName = root[0]["name"].asString();
        if(tagName != tag) {
            update_tag = tagName;
            create_dialog.emit();
        }
    }
    else {
        Logger::error("Failed to check for updates");
    }
}

void Updater::CreateUpdateDialog() {
    auto builder = Gtk::Builder::create_from_file(ResourceManager::get_resource_path("tvtorrent_updatedialog.glade"));
    Gtk::Dialog* dialog;
    builder->get_widget("UpdateDialog", dialog);
    int result = dialog->run();

    switch(result) {
        case Gtk::RESPONSE_YES:
#if defined(WIN32) || defined(WIN64)
        ShellExecute(nullptr, nullptr, (std::string("https://github.com/sheepkill15/tvtorrent/releases/tag/") + update_tag).c_str(), nullptr, nullptr, SW_SHOW);
#endif
            break;
        case Gtk::RESPONSE_NO:
        default:
            break;
    }
    dialog->hide();
    delete dialog;
}

void Updater::Start() {
    create_dialog.connect(sigc::ptr_fun(Updater::CreateUpdateDialog));
    own = std::thread([] { Updater::CheckUpdates(); });
}

void Updater::ForceFinish() {

    own.detach();

}
