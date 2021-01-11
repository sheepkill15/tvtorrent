//
// Created by simon on 2021. jan. 4..
//

#ifndef TVTORRENT_FEED_CONTROL_WINDOW_H
#define TVTORRENT_FEED_CONTROL_WINDOW_H

#include <gtkmm/window.h>
#include <gtkmm/listbox.h>
#include <gtkmm/entry.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/checkbutton.h>
#include <glibmm/dispatcher.h>
#include "feed.h"

class TTMainWindow;

class TTFeedControlWindow {
public:
    explicit TTFeedControlWindow();
    ~TTFeedControlWindow();

    void notify_feed_changed();

private:

    void on_add_filter();
    void on_remove_filter();
    void on_add_feed();
    void on_remove_feed();
    void on_filter_activate(Gtk::ListBoxRow*);

    bool on_filter_pressed(GdkEventButton* ev, Gtk::ListBoxRow*);

    void on_dl_changed();
    void on_vp_changed();
    void on_tvw_changed();

    Feed::Filter* selected_filter{};
    Gtk::ListBox* filter_list{};
    Gtk::ListBox* feed_list{};
    Gtk::ListBox* result_list{};
    Gtk::Entry* download_name{};
    Gtk::Entry* ver_pattern{};
    Gtk::ComboBoxText* tvw_chooser{};

    Glib::Dispatcher m_Dispatcher;

    Gtk::Window* window = nullptr;

    friend class TTMainWindow;

    void on_feed_list_item_click(Gtk::CheckButton*);
    void update_results();

    std::vector<Gtk::CheckButton*> m_Feeds;
    std::unordered_map<size_t, size_t> m_Subscriptions;

    void UpdateFeeds(size_t);
};


#endif //TVTORRENT_FEED_CONTROL_WINDOW_H
