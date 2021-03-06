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
    explicit TTSettingsWindow(TTMainWindow* caller);
    ~TTSettingsWindow();

    Gtk::Window* window = nullptr;
private:
    TTMainWindow* parent;
    Gtk::Entry* dl = nullptr;
    Gtk::Entry* ul = nullptr;
    Gtk::ComboBoxText* theme_select = nullptr;
    Gtk::ComboBoxText* tf_action_select = nullptr;

    void self_destruct();

    inline static size_t m_Running = 0;
};


#endif //TVTORRENT_SETTINGS_WINDOW_H
