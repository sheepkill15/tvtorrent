#ifndef TVTORRENT_MAIN_WINDOW_H
#define TVTORRENT_MAIN_WINDOW_H

#include "feed.h"
#include "gdkmm/event.h"
#include "glibmm/refptr.h"
#include "gtkmm/builder.h"
#include "gtkmm/button.h"
#include "gtkmm/buttonbox.h"
#include "gtkmm/flowbox.h"
#include "gtkmm/headerbar.h"
#include "gtkmm/hvbox.h"
#include "item_window.h"
#include "tv_widget.h"
#include "settings_window.h"
#include <gtkmm/window.h>
#include <gtkmm/scrolledwindow.h>
#include <regex>

class TTFeedControlWindow;

class TTMainWindow : public Gtk::Window {

public:
	explicit TTMainWindow();
	~TTMainWindow() override;

	static void external_torrent(char argv[]);
	void external_torrent_empty();
	void notify(const std::string&);
    void update_limits();

    std::vector<TVWidget*> tvw_list;

protected:
    //void on_button_download();
	void on_button_add();
	void on_button_remove();
	void on_button_settings();
	void on_button_feeds();
	bool on_tvwidget_double_click(GdkEventButton* ev);

	void add_item(const Glib::ustring& name, const Glib::ustring& img_path, const Glib::ustring& default_path);
	void init_items();

	Gtk::FlowBox m_FlowBox;
	Gtk::ScrolledWindow m_ScrolledWindow;
	Gtk::VBox m_VBox;

    Glib::RefPtr<Gtk::Builder> m_refBuilder;

    Glib::Dispatcher m_Dispatcher;
    std::string pending_uri;

    Glib::Dispatcher just_show_please;
};

#endif
