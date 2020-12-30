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
#include <gtkmm/window.h>
#include <gtkmm/scrolledwindow.h>

class TTMainWindow : public Gtk::Window {

public:
	TTMainWindow();
	~TTMainWindow() override;

	void external_torrent(char argv[]);

protected:
    void on_button_download();
	void on_button_add();
	void on_button_remove();
	void on_button_settings();
	bool on_tvwidget_double_click(GdkEventButton* ev);
	void on_item_window_hide(TTItemWindow* window);

	void add_item(const Glib::ustring& name, const Glib::ustring& img_path);
	void init_items();


	Gtk::FlowBox m_FlowBox;
	Gtk::ScrolledWindow m_ScrolledWindow;
	Gtk::VBox m_VBox;

	std::vector<TVWidget*> tvw_list;

	Glib::RefPtr<Gtk::Builder> m_refBuilder;

	Feed m_Feed;
};

#endif
