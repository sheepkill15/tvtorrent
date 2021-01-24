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
    builder->get_widget("ThemeSelect", theme_select);
    builder->get_widget("TFActionSelect", tf_action_select);

    dl->set_text(Glib::ustring::format(SettingsManager::get_settings().dl_limit));
    ul->set_text(Glib::ustring::format(SettingsManager::get_settings().ul_limit));

    for(const auto & i : SettingsManager::theme_to_string) {
        theme_select->append(i);
    }
    theme_select->set_active(SettingsManager::get_settings().selected_theme);

    for(const auto& i : SettingsManager::tf_action_to_string) {
        tf_action_select->append(i);
    }
    tf_action_select->set_active(SettingsManager::get_settings().on_torrent_finish);

    window->ON_HIDE(&TTSettingsWindow::self_destruct);

    window->show();
}

TTSettingsWindow::~TTSettingsWindow() {
    SettingsManager::get_settings().on_torrent_finish = (SettingsManager::TorrentFinishAction) tf_action_select->get_active_row_number();
    SettingsManager::get_settings().selected_theme = (SettingsManager::Theme) theme_select->get_active_row_number();
    char *end;
    SettingsManager::get_settings().ul_limit = strtof(ul->get_text().c_str(), &end);
    SettingsManager::get_settings().dl_limit = strtof(dl->get_text().c_str(), &end);
    parent->update_limits();
    delete window;
}

void TTSettingsWindow::self_destruct() {
    delete this;
}
