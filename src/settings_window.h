//
// Created by simon on 2021. jan. 6..
//

#ifndef TVTORRENT_SETTINGS_WINDOW_H
#define TVTORRENT_SETTINGS_WINDOW_H

#include <gtkmm/window.h>
#include <gtkmm/entry.h>
#include <gtkmm/comboboxtext.h>

class TTMainWindow;

class TTSettingsWindow {
public:
    TTSettingsWindow(TTMainWindow* caller);
    ~TTSettingsWindow();

    Gtk::Window* window = nullptr;
private:
    TTMainWindow* parent;
    Gtk::Entry* dl = nullptr;
    Gtk::Entry* ul = nullptr;
    Gtk::ComboBoxText* theme_select = nullptr;

    void on_dl_change();
    void on_ul_change();

    void on_theme_change();
};


#endif //TVTORRENT_SETTINGS_WINDOW_H
