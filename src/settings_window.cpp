//
// Created by simon on 2021. jan. 6..
//

#include "settings_window.h"
#include "resource_manager.h"
#include "settings_manager.h"
#include "macros.h"
#include "main_window.h"
#include <cstdlib>

TTSettingsWindow::TTSettingsWindow(TTMainWindow* caller)
    :parent(caller)
{
    auto builder = Gtk::Builder::create_from_file(ResourceManager::get_resource_path("tvtorrent_settings.glade"));
    builder->get_widget("SettingsWindow", window);

    builder->get_widget("DL", dl);
    builder->get_widget("UL", ul);

    dl->set_text(Glib::ustring::format(SettingsManager::get_settings().dl_limit));
    ul->set_text(Glib::ustring::format(SettingsManager::get_settings().ul_limit));

    dl->ON_CHANGE(&TTSettingsWindow::on_dl_change);
    ul->ON_CHANGE(&TTSettingsWindow::on_ul_change);
    window->show();
}

TTSettingsWindow::~TTSettingsWindow() {
    delete window;
}

void TTSettingsWindow::on_dl_change() {
    char* end;
    SettingsManager::get_settings().dl_limit = strtof(dl->get_text().c_str(), &end);
    parent->update_limits();
}

void TTSettingsWindow::on_ul_change() {
    char* end;
    SettingsManager::get_settings().ul_limit = strtof(ul->get_text().c_str(), &end);
    parent->update_limits();
}
